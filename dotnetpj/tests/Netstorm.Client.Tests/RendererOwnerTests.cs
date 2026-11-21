using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 기존 dirty/합성 본체와 새 Renderer 소유자의 실제 상태/픽셀/출력 순서를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>원문288출력과 현재 장면·캐시·수집·글자·커서·실패 접두 검사다.</summary>
public sealed class RendererOwnerTests
{
    /// <summary>독립 CPP 검사와 같은 한 픽셀 VFX 입력이다.</summary>
    private static byte[] PixelShape(byte color)
    {
        byte[] bytes = new byte[43]; BinaryPrimitives.WriteUInt32LittleEndian(bytes, 0x30312e31); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(4), 1); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(8), 16); bytes[40] = 2; bytes[41] = color; return bytes;
    }
    /// <summary>두 물리 프레임이 같은 한 픽셀을 덮는 독립 입력이다.</summary>
    private static byte[] TwoFrames()
    {
        byte[] bytes = new byte[78]; BinaryPrimitives.WriteUInt32LittleEndian(bytes, 0x30312e31); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(4), 2); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(8), 24); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(16), 51); bytes[48] = bytes[75] = 2; bytes[49] = 7; bytes[76] = 9; return bytes;
    }
    /// <summary>A만 실제 한 픽셀 포인터와3픽셀 전진을 갖는 원문 chfnt 입력이다.</summary>
    private static BitmapFont Font()
    {
        byte[] shape = PixelShape(100), bytes = new byte[BitmapFont.PayloadStart + shape.Length]; BitmapFont.Signature.CopyTo(bytes);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(16), 1); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(20), 5); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(24), 4); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(28), 1); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(32), 256);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(BitmapFont.TableStart + 65 * 4), 3); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(BitmapFont.TableStart + BitmapFont.TableBytes * 2 + 65 * 4), 3); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(BitmapFont.TableStart + BitmapFont.TableBytes * 3 + 65 * 4), (uint)shape.Length); shape.CopyTo(bytes, BitmapFont.PayloadStart); return new(bytes);
    }
    /// <summary>장면/정렬 순서만 기록하고 한 픽셀을 실제 버퍼에 그린다.</summary>
    private static RenderSprite Procedure(string name, short depth, byte color, List<string> trace) => new()
    {
        Clip = new(0, 0, 8, 4), Order = new(0, 0, depth), Procedure = new(new(0, 0, 1, 1), (pixels, pitch, height, clip) => { trace.Add(name); pixels[0] = color; }),
    };
    /// <summary>원문의 U 호출과 같은 정수 좌표 표기다.</summary>
    private static string Update(ScreenRect rect) => $"U:{rect.Left}:{rect.Top}:{rect.Right}:{rect.Bottom}";

    /// <summary>두 요청/빈표/첫flags/세 화면비의288개 독립 관찰과 실제 출력 호출을 전수 대조한다.</summary>
    [Fact]
    public void EveryNativePresentationDirtySequenceMatches()
    {
        string[][] rows = X86Fixture.Read("renderer-present-dirty"); Assert.Equal(288, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] words = row.Take(17).Select(X86Fixture.UInt).ToArray(); var renderer = new Renderer((int)words[0], (int)words[1]); byte[] pixels = new byte[checked((int)(words[0] * words[1]))];
            renderer.Draw(pixels, (int)words[0]); renderer.Present(_ => { }); renderer.CameraDirtyEntries.Add(new(new(2, 3, 7, 8), words[6])); renderer.ClearDirty();
            if (words[5] != 0) renderer.CameraDirtyEntries.Add(new(new(2, 3, 7, 8), words[6])); if (words[5] == 2) renderer.CameraDirtyEntries.Add(new(new(20, 30, 40, 50), words[7])); renderer.CameraRedraw.Value = words[2]; renderer.BackgroundRedraw.Value = words[3];
            int expectedDraws = (words[5] > 0 && (words[6] & 1) == 0 ? 1 : 0) + (words[5] > 1 && (words[7] & 1) == 0 ? 1 : 0); Assert.Equal(expectedDraws, renderer.Draw(pixels, (int)words[0]).Count); Assert.Equal(words[2], renderer.CameraRedraw.Value); Assert.Equal(words[3], renderer.BackgroundRedraw.Value);
            var updates = new List<string>(); renderer.Present(rect => { Assert.Equal(0u, renderer.CameraRedraw.Value); Assert.Equal(0u, renderer.BackgroundRedraw.Value); Assert.Equal(words[2] != 0 || words[3] != 0 ? 1 : (int)words[5], renderer.CameraDirtyEntries.Count); updates.Add(Update(rect)); });
            string expected = string.Join(';', row[17].Split(';').Where(item => item.StartsWith("U:", StringComparison.Ordinal))); bool same = string.Join(';', updates) == expected && renderer.CameraRedraw.Value == words[8] && renderer.BackgroundRedraw.Value == words[9];
            Assert.Empty(renderer.CameraDirtyEntries); Assert.Empty(renderer.Draw(pixels, (int)words[0])); renderer.Present(_ => { }); return same;
        });
    }
    /// <summary>이전 위치의 배경을 복원하고 새 위치만 그리며 커서의 동일 위치 force를 소비한다.</summary>
    [Fact]
    public void MotionAndForcedCursorInvalidateActualOldAndNewPixels()
    {
        var renderer = new Renderer(8, 4); byte[] pixels = new byte[32]; var database = new ShapeDatabase(PixelShape(7)); var sprite = new RenderSprite { Database = database, Clip = new(0, 0, 8, 4), X = 1, Y = 1 };
        renderer.SetScene([sprite]); renderer.Draw(pixels, 8); renderer.Present(_ => { }); Assert.Equal(7, pixels[9]); renderer.SetScene([sprite with { X = 4 }]); Assert.Equal(2, renderer.Draw(pixels, 8).Count); Assert.Equal(0, pixels[9]); Assert.Equal(7, pixels[12]); renderer.Present(_ => { });
        var cursor = new IndexedImage { Width = 1, Height = 1, Indices = [99], Opacity = [255] }; renderer.SetSoftwareCursor(cursor, new(2, 2)); renderer.Draw(pixels, 8); renderer.Present(_ => { }); Assert.Equal(99, pixels[18]);
        renderer.SetSoftwareCursor(cursor, new(2, 2)); Assert.Empty(renderer.CameraDirtyEntries); renderer.SetSoftwareCursor(cursor, new(2, 2), true); Assert.NotEmpty(renderer.CameraDirtyEntries); renderer.Draw(pixels, 8); renderer.Present(_ => { }); renderer.SetSoftwareCursor(null, default); renderer.Draw(pixels, 8); Assert.Equal(0, pixels[18]); renderer.Present(_ => { });
    }
    /// <summary>정적/추가 항목을 같은 원문 키로 정렬하고 SceneImage는 재수집 없이 마지막 캐시만 쓴다.</summary>
    [Fact]
    public void ExtraCollectionSharesSortAndSceneImageDoesNotRecollect()
    {
        var renderer = new Renderer(8, 4); var trace = new List<string>(); int collections = 0; byte[] pixels = new byte[32];
        renderer.SetScene([Procedure("A", 1, 1, trace), Procedure("B", 2, 2, trace)]); renderer.SetExtraCollection((_, _) => { collections++; return new[] { Procedure("E", 3, 3, trace) }; });
        renderer.Draw(pixels, 8); renderer.Present(_ => { }); Assert.Equal(new[] { "E", "B", "A" }, trace); Assert.Equal(1, pixels[0]); trace.Clear(); IndexedImage image = renderer.SceneImage(); Assert.Equal(1, collections); Assert.Equal(1, image.Indices[0]); Assert.Equal(new[] { "E", "B", "A" }, trace);
        renderer.SetExtraCollection(null); Assert.False(renderer.HasExtraCollection); trace.Clear(); renderer.SceneImage(); Assert.Equal(new[] { "B", "A" }, trace); Assert.Equal(1, collections);
    }
    /// <summary>수집 중 설치된 실제 submit을 재조회하고 분리 미리보기/UI/글자를 중복 출력하지 않는다.</summary>
    [Fact]
    public void NativeSubmitRequeriesCurrentDeviceAndBypassesSeparateScene()
    {
        var renderer = new Renderer(8, 4); byte[] pixels = new byte[32]; var trace = new List<string>(); int extra = 0;
        renderer.SetScene([Procedure("sprite", 1, 1, trace)], [new RenderText()]); renderer.SetExtraCollection((_, _) => { extra++; return []; }); renderer.SetOverlayProcedure(new(new(0, 0, 8, 4), (_, _, _, _) => trace.Add("overlay")));
        renderer.SetSoftwareCursor(new() { Width = 1, Height = 1, Indices = [99], Opacity = [255] }, default); var world = new RendererWorldDrawing();
        world.Collect = (_, _) => { trace.Add("collect"); world.Submit = (buffer, _, _, _) => { trace.Add("submit"); buffer[0] = 17; }; return 7; }; world.Background = (_, flags, covered) => { Assert.Equal(0u, flags); Assert.Equal(7u, covered); trace.Add("background"); };
        renderer.Draw(pixels, 8, world: world); renderer.Present(_ => { }); Assert.Equal(new[] { "collect", "background", "submit" }, trace); Assert.Equal(0, extra); Assert.Equal(99, pixels[0]);
    }
    /// <summary>같은 DB의 해제 이미지는 명시 반납 전까지 유지하고 실제 자산 변경 후만 다시 푼다.</summary>
    [Fact]
    public void DecodedImageCacheUsesDatabaseIdentityAndExplicitClear()
    {
        byte[] bytes = PixelShape(7); var database = new ShapeDatabase(bytes); var renderer = new Renderer(8, 4); byte[] pixels = new byte[32]; renderer.SetScene([new() { Database = database, Clip = new(0, 0, 8, 4) }]); renderer.Draw(pixels, 8); renderer.Present(_ => { }); Assert.Equal(7, pixels[0]);
        bytes[41] = 9; renderer.InvalidateAll(); renderer.Draw(pixels, 8); renderer.Present(_ => { }); Assert.Equal(7, pixels[0]); renderer.ClearDecodedImages(); renderer.InvalidateAll(); renderer.Draw(pixels, 8); renderer.Present(_ => { }); Assert.Equal(9, pixels[0]);
        renderer.SetScene([new() { Database = new ShapeDatabase(PixelShape(11)), Clip = new(0, 0, 8, 4) }]); renderer.Draw(pixels, 8); renderer.Present(_ => { }); Assert.Equal(11, pixels[0]);
    }
    /// <summary>같은 항목의 후속 프레임과 색표는 값 복사하며 호출자 배열 변경으로 바뀌지 않는다.</summary>
    [Fact]
    public void LayersAndColorMapsKeepValueSemantics()
    {
        var renderer = new Renderer(8, 4); byte[] pixels = new byte[32], colors = new byte[256]; colors[7] = 17; colors[9] = 19; int[] following = [1];
        renderer.SetScene([new() { Database = new ShapeDatabase(TwoFrames()), Clip = new(0, 0, 8, 4), Colors = colors, FollowingFrames = following }]); colors[9] = 66; following[0] = 0;
        renderer.Draw(pixels, 8); renderer.Present(_ => { }); Assert.Equal(19, pixels[0]); Assert.Equal(19, renderer.SceneImage().Indices[0]);
    }
    /// <summary>원문 코드 포인터/명시 NUL/그림자/4방향/현재 clip을 기존 실제 글리프 본체로 출력한다.</summary>
    [Fact]
    public void TextUsesActualGlyphPassesAndSceneImageExcludesUiText()
    {
        var renderer = new Renderer(8, 4); byte[] pixels = new byte[32]; renderer.SetScene([], [new() { Font = Font(), Bytes = [65, 0, 65], X = 1, Y = 1, Color = 9, ShadowColor = 3, Shadow = true, Outline = true, ShadowOffset = new(1, 1), Clip = new(0, 0, 6, 3) }]);
        renderer.Draw(pixels, 8); renderer.Present(_ => { }); Assert.Equal(9, pixels[9]); Assert.Equal(9, pixels[12]); Assert.Equal(3, pixels[18]); Assert.Equal(3, pixels[21]); Assert.Equal(3, pixels[8]); Assert.All(pixels.Skip(24), value => Assert.Equal(0, value)); Assert.All(renderer.SceneImage().Indices, value => Assert.Equal(0, value));
    }
    /// <summary>출력 실패는 요청 소비/성공 카운터 접두를 유지하고 실제 그리기를 다시 시작하지 않는다.</summary>
    [Fact]
    public void PresentFailureKeepsCurrentTableAndPaintedPrefix()
    {
        var renderer = new Renderer(8, 4); byte[] pixels = new byte[32]; renderer.Draw(pixels, 8); renderer.Present(_ => { }); renderer.Invalidate(new(1, 1, 2, 2)); renderer.Draw(pixels, 8); renderer.CameraRedraw.Value = 1; renderer.BackgroundRedraw.Value = 7;
        Assert.Throws<IOException>(() => renderer.Present(_ => throw new IOException("명시출력중단"))); Assert.Equal(0u, renderer.CameraRedraw.Value); Assert.Equal(0u, renderer.BackgroundRedraw.Value); Assert.Equal(1ul, renderer.PresentCount); Assert.Single(renderer.CameraDirtyEntries); Assert.Equal(new ScreenRect(0, 0, 8, 4), renderer.CameraDirtyEntries[0].Rect); Assert.Throws<InvalidOperationException>(() => renderer.Draw(pixels, 8));
        var updates = new List<ScreenRect>(); renderer.Present(updates.Add); Assert.Equal(new[] { new ScreenRect(0, 0, 8, 4) }, updates); Assert.Equal(2ul, renderer.PresentCount); Assert.Empty(renderer.CameraDirtyEntries); Assert.Empty(renderer.Draw(pixels, 8)); renderer.Present(_ => { });
    }
    /// <summary>화면 이동의 노출 띠만 합성하면서 출력 요청은 원문대로 전체 화면을 내보낸다.</summary>
    [Fact]
    public void CameraRequestPresentsFullWithoutRecomposingInterior()
    {
        var renderer = new Renderer(20, 10); byte[] pixels = new byte[200]; renderer.Draw(pixels, 20); renderer.Present(_ => { }); Array.Fill(pixels, (byte)7); var collected = new List<ScreenRect>();
        var world = new RendererWorldDrawing { Collect = (rect, _) => { collected.Add(rect); return 0; }, Background = (rect, _, _) =>
        {
            // 실제 배경 장치는 현재 노출 띠의 행만 복원한다.
            for (int y = rect.Top; y < rect.Bottom; y++) pixels.AsSpan(y * 20 + rect.Left, rect.Right - rect.Left).Fill(9);
        } };
        renderer.Invalidate(new(18, 0, 20, 10)); renderer.CameraRedraw.Value = 1; renderer.Draw(pixels, 20, world: world); var updates = new List<ScreenRect>(); renderer.Present(updates.Add);
        Assert.Equal(new[] { new ScreenRect(18, 0, 20, 10) }, collected); Assert.Equal(new[] { new ScreenRect(0, 0, 20, 10) }, updates); Assert.Equal(7, pixels[17]); Assert.Equal(9, pixels[18]); Assert.Equal(7, pixels[197]); Assert.Equal(9, pixels[199]);
    }
    /// <summary>실제 표의 빈 유효 항목은 합성하지 않아도 Present 표에서 임의로 제거하지 않는다.</summary>
    [Fact]
    public void ClippingMutatesActualTableAndPreservesEmptyUpdate()
    {
        var renderer = new Renderer(20, 10); byte[] pixels = new byte[200]; renderer.Draw(pixels, 20); renderer.Present(_ => { }); renderer.CameraDirtyEntries.Add(new(new(-2, -3, 7, 8), 0)); renderer.CameraDirtyEntries.Add(new(new(21, 11, 30, 20), 0));
        Assert.Equal(new[] { new ScreenRect(0, 0, 7, 8) }, renderer.Draw(pixels, 20)); var updates = new List<ScreenRect>(); renderer.Present(updates.Add); Assert.Equal(new[] { new ScreenRect(0, 0, 7, 8), new ScreenRect(20, 10, 20, 10) }, updates);
    }
    /// <summary>Session 없는 빈 취소도 실제 현재 카메라/위치 준비/Renderer dirty를 실행한다.</summary>
    [Fact]
    public void EmptyCursorCancelUsesActualCameraProjectionAndRendererDirty()
    {
        var game = new ClientGameplayGlobals(); game.Camera.InitializeNativeRenderer(128, 96, 256, new(), game.MapRevision); RawCameraView.SetWorld(game.Camera.Snapshot, 5, 5); var renderer = new Renderer(128, 96); byte[] pixels = new byte[128 * 96]; renderer.Draw(pixels, 128); renderer.Present(_ => { }); game.Cursor.X = game.Cursor.Y = 10;
        var pool = new RawSidPool(24000); OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        var placement = new RawCursorPlacement(pool, types, ReadOnlyMemory<ushort>.Empty, game.Cursor, game.CursorGlobals.Lifecycle, game.CursorGlobals.Placement, new() { Project = (x, y) => RawCameraView.Project(game.Camera.Snapshot, x, y), Invalidate = renderer.Invalidate }); var hooks = new CursorLifecycleHooks(); placement.BindLifecycle(hooks); new RawCursorLifecycle(pool, types, game.Cursor, game.CursorGlobals.Lifecycle, hooks).Cancel();
        Assert.Single(renderer.CameraDirtyEntries); Assert.Equal(new ScreenRect(78, 53, 86, 69), renderer.CameraDirtyEntries[0].Rect); Assert.Equal(new[] { new ScreenRect(78, 53, 86, 69) }, renderer.Draw(pixels, 128)); renderer.Present(_ => { }); Assert.Throws<InvalidOperationException>(() => game.RequireSession());
    }
}
