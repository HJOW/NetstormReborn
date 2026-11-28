using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // 같은 실제 생산 자식 본체의 생성/측정/입력 장치다.

/// <summary>Source RawProductionControls 생성자의 실제 자식 조립 부분이다.</summary>
public sealed partial class HostProductionWindows
{
    /// <summary>같은 원문 전체 TYPE/패턴/자산과 공통 문자/노드만 연결한다.</summary>
    private void BuildChildren(MarkupGameAssets assets)
    {
        var storage = _context.Storage; var game = _context.Gameplay; var frames = storage.Frames.Select((value, index) => new PriestPlainCanonType { Frames = value, DefaultFrame = index < 70 || index - 70 >= assets.Definitions.Count ? 0 : assets.Definitions[index - 70].SpecialFrames.DefaultFrame }).ToArray();
        var geometry = new PriestPlacementGeometryState(); var measure = new RawCanonPixelShape(storage.Pool, storage.Types, frames, storage.Shapes, storage.Types.Select(type => new PriestTypeHotspot(type.HotspotX, type.HotspotY)).ToArray(), geometry, new()); var bindings = new List<ImageGumpAssetBinding>();
        // 같은 최초 자산 표에 실제 SHP가 있는 TYPE만 연결한다.
        for (int type = 70; type < storage.Types.Count && type - 70 < assets.Definitions.Count; type++) { var record = storage.Types[type]; bindings.Add(new() { Type = (uint)type, Block = type - 70, FrameCount = storage.Frames[type].Codes.Count, Flags = record.Flags1, HotspotX = record.HotspotX, HotspotY = record.HotspotY }); }
        var images = new ImageGumpAssets(assets.Shapes, bindings); uint[] maps;
        if (_context.OwnerColorTables is { } tables)
        {
            if (!MemoryMarshal.TryGetArray(tables.Tokens, out ArraySegment<uint> segment) || segment.Offset != 0 || segment.Array is null || segment.Count != segment.Array.Length) throw new InvalidOperationException("생산 현재 색표의 실제 전체 배열 필요"); maps = segment.Array;
        }
        else
        {
            maps = new uint[storage.Types.Count];
            // Source의 기존 독립 UI 입력만 지역 일반 표를 소유한다.
            foreach (uint type in new uint[] { 82, 155, 158, 164, 135, 133, 156, 73, 170, 146, 144, 126 }) { maps[type] = OwnerColorTableState.TokenBase + type * OwnerColorTableState.TableBytes; _localColors.Add(type, OwnerColorMapBuilder.BuildStandardOwnerColorMaps(type, 82)); }
            // 전용 지역 표는 실제 파일 팔레트가 설치될 때 생성한다.
            foreach (uint type in new uint[] { 111, 141, 137, 94, 97 }) maps[type] = OwnerColorTableState.TokenBase + type * OwnerColorTableState.TableBytes;
        }
        _imageState.OwnerMaps = maps; var hitClip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(hitClip, new(0, 0, 200, 200)); var hitTarget = new GumpPixelTarget(Enumerable.Repeat((byte)255, 40000).ToArray(), 200, 200, hitClip); var hit = new ImageGumpPixelTarget(hitTarget, images, new byte[256], Color);
        var ih = new ImageGumpHooks { Node = token => Node(token) as ImageGumpNode, Position = _tree.Collection.Position, Attach = _tree.Tree.Attach, Hit = hit.Hooks(new()).Hit, Draw = request => Need(_activeImage?.Draw)(request), Color = token => Need(_activeImage?.Color)(token) }; ImageGumpAssets.BindImageGumpAssets(ih, images); _image = new(_tree.Create, _tree.Factory, _imageState, ih);
        _grid = new(_tree.Create, _tree.Factory, storage.Pool, game.ProductionFinder, new() { CursorPoint = () => new(game.Cursor.ScreenX, game.Cursor.ScreenY), CursorFlags = () => game.Cursor.InputFlags }, new()
        {
            Grid = token => Node(token) as InventoryGridNode, Item = token => Node(token) as InventoryItemNode, ClearChildren = ClearChildren, BaseAttach = _tree.Tree.Attach,
            AllocateItem = bytes => bytes == 0xc0 ? Allocate(new InventoryItemNode()) : throw new ArgumentOutOfRangeException(nameof(bytes), "생산 항목 할당 크기"), Attach = VirtualAttach, Push = _tree.Tree.Push, Bind = UnbindLostGridParent, Idle = _tree.Lifetime.SetIdle, Now = _devices.Now,
            ContentParent = sid => Word(sid, 16), Forward = _tree.Events.Bubble, BaseHandle = _devices.BaseHandle, Local = (token, _, point) => _tree.Collection.ScreenToLocal(token, point), Inside = (token, point) => { var node = _tree.Node(token)!; return point.X >= 0 && point.Y >= 0 && point.X < node.Width && point.Y < node.Height; },
            CancelCursor = () => { _ = game.RequireSession(); Need(_devices.CancelCursor)(); }, Assertion = message => throw new InvalidOperationException(message), Redraw = _devices.Redraw,
            ClearBlink = content => { if (content >= storage.Pool.Capacity || content > ushort.MaxValue) throw new ArgumentOutOfRangeException(nameof(content), "생산 점멸 내용물 SID 범위"); storage.Pool.AllocatedBytes((ushort)content)[21] &= 0xfd; },
        });
        var itemHooks = new InventoryItemHooks
        {
            Node = token => Node(token) as InventoryItemNode, Base = _tree.Node, BaseAttach = _tree.Tree.Attach, ClearChildren = ClearChildren, Cleanup = _tree.Lifetime.Cleanup, Robust = (sid, _) => sid < storage.Pool.Capacity && storage.Pool.Slot((ushort)sid)[10] == game.ProductionFinder.ContainedType, Assertion = message => throw new InvalidOperationException(message),
            Allocate = bytes => Allocate(bytes == 0xb4 ? new ImageGumpNode() : bytes == 0xa4 ? new TextGumpNode() : throw new ArgumentOutOfRangeException(nameof(bytes), "생산 자식 할당 크기")), Image = (token, type, frame, mode) => _image.Construct(token, type, unchecked((uint)frame), mode), Text = token => _text.Single.Construct(token, 0, 0, 0, 0), StyledText = Style, FitText = token => _text.Single.Fit(token), Attach = VirtualAttach,
            CostUnits = type => game.RequireSession().Reward.TypeCostUnits(type), Money = Money, Position = _tree.Collection.Position, Background = (rect, flags, type, frame) => (_activeBackground ?? throw new InvalidOperationException("생산 장식 실제 표면 없음")).Texture(rect, flags, type, frame),
            Local = (token, _, x, y) => _tree.Collection.ScreenToLocal(token, new(unchecked((int)x), unchecked((int)y))), KnowledgeOpen = () => game.Cursor.LockDepth == 0 ? 1u : 0, ImageHit = _image.Hit, Denied = (first, flags) => first == _globals.PolicyFirst && unchecked((byte)flags) == _globals.PolicyByte ? 1u : 0, Tell = _devices.Tell, Forward = _tree.Events.Bubble,
            About = (token, x, y) => Need(_devices.About)((InventoryItemNode)Node(token)!, new(unchecked((int)x), unchecked((int)y)), sid => _item.Affordable(sid) != 0),
        };
        InventoryItemGump.BindAssets(itemHooks, OriginalEdition.Patch1078, assets.Definitions, frames, geometry, measure);
        _item = new(storage.Pool, new() { LocalOwner = () => game.Player.BattleOwner, Editor = () => game.World.Editor != 0, StockEnabled = () => _globals.StockEnabled, InputDisabled = () => _inputDisabled, LastType = _lastType, BridgeType = () => _globals.BridgeType, PixelsX = () => 16, PixelsY = () => 11, BackgroundType = () => _globals.BackgroundType, BackgroundFrame = () => _globals.BackgroundFrame }, itemHooks);
        _hint = new(_tree.Create, _text.ProductionHints, new()
        {
            Node = token => Node(token) as ProductionHintNode, Now = _devices.Now, Allocate = bytes => bytes == 0xb8 ? Allocate(new ProductionHintNode()) : throw new ArgumentOutOfRangeException(nameof(bytes), "생산 안내 할당 크기"), ConstructText = token => _text.Single.Construct(token, 0, 0, 6, 0), Kill = token => { _tree.Lifetime.Kill(token); _devices.Changed(); },
            Duplicate = value => { if (_hintStringSerial == uint.MaxValue) throw new OverflowException("생산 안내 문자열 토큰 소진"); uint token = ++_hintStringSerial; _hintStrings.Add(token, value); return token; }, String = token => _hintStrings[token], Free = token => { if (!_hintStrings.Remove(token)) throw new InvalidOperationException("생산 안내 문자열 중복 반납"); }, Translate = value => Need(_devices.Translate)(value), Style = Style, Fit = _text.Single.Fit, Idle = _tree.Lifetime.SetIdle, Bounds = _tree.Collection.Bounds, Position = _tree.Collection.Position, Draw = (head, point) => _text.Style.Draw(head, point), Attach = _tree.Tree.Attach, DeleteText = _text.Single.DeleteScalar,
        });
        var dividerHooks = new HudDockHooks
        {
            Node = _tree.Node, Divider = token => Node(token) as HudDividerNode, ConstructBackground = (token, width, height, flags, type, frame) => _tree.Backgrounds.Construct(token, width, height, flags, 1, type, frame), Activate = (token, value) => _tree.Lifetime.SetOverlay(token, value != 0), BaseAttach = _tree.Tree.Attach, Push = _tree.Tree.Push, Attach = VirtualAttach, Move = _tree.Tree.Move, ResolveSerial = Reference,
            Bubble = (token, code, first, second) => _tree.Events.Bubble(token, code, first, second), ClosePopups = () => Need(_devices.ClosePopups)(),
        };
        _divider = new(() => _tree.Create.ScreenWidth, () => _tree.Create.ScreenHeight, () => 55, dividerHooks); _dividerInput = new(_tree.Collection, _tree.Input, _tree.Tree, new() { Divider = token => Node(token) as HudDividerNode, Base = _tree.Node, BaseHandle = _devices.BaseHandle, Poll = code => Need(_devices.Poll)(code), Cursor = kind => Need(_devices.Cursor)(kind), MenuInstances = () => Need(_devices.MenuInstances)() });
        _imageAssets = images;
    }
    /// <summary>같은 실제 이미지 자산이며 현재 출력 표면은 호출 동안만 빌린다.</summary>
    private ImageGumpAssets _imageAssets = null!;
    /// <summary>원문0부모는 무동작이며 실제 Form 작성 뒤 필드만 게시한다.</summary>
    private void BindForm(uint token, uint parent)
    {
        if (parent == 0) return; uint form = Need(_devices.BindForm)(token, parent); var node = Node(token) ?? throw new InvalidOperationException("생산 Form 실제 Gump 없음"); node.Flags |= 0x40; node.Object = parent; node.Form = form;
    }
    /// <summary>부모 소실 통지에서 연결 표식과 Form 번호만 지운다. 부모 번호는 Idle 비교에 남기고 Form은 통지 뒤 원래 종속 삭제에 맡긴다.</summary>
    /// <remarks>cpppj CP1386의 UnbindLostParent에 맞춰 조기 Form 반납과 소멸한 부모로의 Idle 재연결을 막는다. 일반 Bind의 부모0 무동작은 유지한다.</remarks>
    private void UnbindLostGridParent(uint token, uint parent)
    {
        if (parent != 0) { BindForm(token, parent); return; }
        var node = Node(token) ?? throw new InvalidOperationException("생산 격자의 실제 Gump 없음");
        node.Flags &= ~0x40u; node.Form = 0;
    }
    /// <summary>같은 전체 등록표에서 현재 serial로 살아 있는 노드만 찾는다.</summary>
    private uint Reference(uint serial)
    {
        // 루트 칸0은 원문 검색 대상이 아니다.
        foreach (uint token in _tree.Create.Entries.Skip(1)) if (token != 0 && _tree.Node(token) is { } node && node.Serial == serial && (node.Flags & 0x10000) == 0) return token; return 0;
    }
}
