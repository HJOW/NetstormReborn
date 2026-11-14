// 타입·프레임 자료와 카메라에서 객체 출력 요청을 만드는 처리기이다.
using Netstorm.Core.Display;

namespace Netstorm.Core.Original;

// 표시 수집 모듈을 담는 네임스페이스다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.

// 타입·프레임 자료와 카메라에서 객체 출력 요청을 만드는 처리기이다. 클라이언트 장치에는 의존하지 않으며 sink에만 전달한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.
public sealed class SquidDisplay : IRawUnpopDisplay
{
    /// <summary>표시 레이어와 extra의 선택·표식 비트다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private const uint DoubleFrames = 0x440000u;

    /// <summary>그림자 레이어 비트다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private const uint Shadow = 0x40000u;

    /// <summary>선택 표식 비트다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private const uint Marker = 0x10u;

    /// <summary>실제 마나 가상 표다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private const uint PatchManaVtable = 0x50c3b0u;

    /// <summary>볼트 가상 표다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private const uint BoltVtable = 0x509810u;

    /// <summary>실제 간헐천 가상 표다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private const uint PatchGeyserVtable = 0x50da88u;

    /// <summary>원문 lazy 사각형의 위·아래 픽셀 여백이다. 확대율을 곱하지 않는다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private const int GeyserVerticalInset = 50;

    /// <summary>소용돌이 부분 갱신 소스다. 원문 8c 진단 뒤 같은 파생을 선택한다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private const uint MetaVtable = 0x50c9f0u;

    /// <summary>부호 없는 DWORD 감김 한계다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private const uint VortexFlag = 0x2000u;

    /// <summary>판본별 필드 배치와 동작을 선택하는 원본 버전이다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private readonly OriginalEdition _edition;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>프레임의 크기·그림 데이터를 조회하는 셰이프 표다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<SquidDisplayShape> _shapes;

    /// <summary>계산한 그림 요청을 실제 출력에 전달하는 대상이다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private readonly SquidDisplaySink _sink;

    /// <summary>출력 범위·원점·카메라를 공유하는 뷰다. 생성자에서 SetView로 둔다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    private SquidDisplayView _view = null!;

    /// <summary>타입 번호와 같은 인덱스의 SHP 목록을 받는다. 두 목록과 sink는 이 객체보다 오래 살아야 한다.</summary>
    /// <param name="edition">따를 원본 판본이다.</param>
    /// <param name="types">타입 표다.</param>
    /// <param name="shapes">셰이프 표다.</param>
    /// <param name="sink">출력 대상이다.</param>
    /// <param name="view">초기 카메라·화면이다.</param>
    public SquidDisplay(OriginalEdition edition, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<SquidDisplayShape> shapes, SquidDisplaySink sink, SquidDisplayView view)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(shapes);
        ArgumentNullException.ThrowIfNull(sink);
        ArgumentNullException.ThrowIfNull(view);
        if (types.Count != shapes.Count)
        {
            throw new ArgumentException("표시 타입·SHP 목록 크기 오류");
        }

        _edition = edition;
        _types = types;
        _shapes = shapes;
        _sink = sink;
        SetView(view);
    }

    /// <summary>카메라·viewport 변경은 호출자의 전체 갱신과 함께 사용한다.</summary>
    /// <param name="view">바꿀 카메라·화면이다.</param>
    public void SetView(SquidDisplayView view)
    {
        ArgumentNullException.ThrowIfNull(view);
        if (view.Zoom <= 0 || view.Viewport.Left > view.Viewport.Right || view.Viewport.Top > view.Viewport.Bottom)
        {
            throw new ArgumentException("표시 카메라·viewport 오류");
        }

        _view = view;
    }

    /// <summary>프레임 연결부가 같은 판본·타입 목록을 쓰는지 구성 때 확인한다.</summary>
    /// <param name="edition">연결할 판본이다.</param>
    /// <param name="types">연결할 타입 표다.</param>
    public void ValidateFrameBinding(OriginalEdition edition, IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (edition != _edition || types.Count != _types.Count)
        {
            throw new ArgumentException("프레임 표시 판본·타입 목록 불일치");
        }

        // 표시 레이어·유효성 및 공간 재등록이 사용하는 타입 필드를 비교한다.
        for (int i = 0; i < types.Count; i++)
        {
            OriginalTypeRecord a = types[i];
            OriginalTypeRecord b = _types[i];
            if (a.Flags1 != b.Flags1 || a.Flags2 != b.Flags2 || a.MaxHitPoints != b.MaxHitPoints || a.FootX != b.FootX || a.FootY != b.FootY)
            {
                throw new ArgumentException("프레임 표시 타입 내용 불일치");
            }
        }
    }

    /// <summary>해시 단계용 헤더는 표시 억제와 무관하게 읽는다. 그림자·abstract 물리 프레임 순서도 그대로 유지한다.</summary>
    /// <param name="edition">읽을 판본이다.</param>
    /// <param name="bytes">읽을 슬롯 바이트다.</param>
    /// <param name="frame">읽을 프레임 번호다.</param>
    /// <returns>셀 가로·세로다.</returns>
    public float[] FrameSize(OriginalEdition edition, ReadOnlySpan<byte> bytes, int frame)
    {
        if (edition != _edition)
        {
            throw new ArgumentException("프레임 SHP 판본 불일치");
        }

        uint type = Read(bytes, 10, 1);
        if (type >= (uint)_shapes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "프레임 SHP 타입 번호 범위 오류");
        }

        if (_edition == OriginalEdition.Patch1078)
        {
            var hooks = new RiftShapeAccessHooks
            {
                TypeNumber = selected => selected.Token,
                FrameError = (number, savedFrame, abstractValue) => _sink.FrameError(number, savedFrame, abstractValue),
                Assertion = (expression, file, line) => _sink.Assertion(expression, file, line),
                Shape = (selected, savedFrame) =>
                {
                    SquidDisplayShape current = _shapes[(int)selected];
                    if (!current.Loaded || savedFrame < 0 || savedFrame >= current.Frames.Count)
                    {
                        throw new ArgumentOutOfRangeException(nameof(savedFrame), "진단 복귀 뒤 SHP 물리 범위 오류");
                    }

                    SquidDisplayFrame header = current.Frames[savedFrame];
                    return new float[] { header.CellWidth, header.CellHeight };
                },
            };
            // 원문 004ace40은 raw abstract 대신 고정 인자 0을 전달한다.
            return RawRiftShapeAccess.ReadFrame(new RawSquidDisplayTypeView(type, _types[(int)type].Flags1, _shapes[(int)type].FrameCount, _shapes[(int)type].Loaded, _types[(int)type].MaxHitPoints), frame, 0u, hooks);
        }

        SquidDisplayShape shape = _shapes[(int)type];
        if (!shape.Loaded)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "프레임 SHP 조회 실패");
        }

        if (frame < 0 || frame >= shape.Frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), "프레임 SHP 물리 범위 오류");
        }

        SquidDisplayFrame header = shape.Frames[frame];
        return [header.CellWidth, header.CellHeight];
    }

    /// <summary>같은 표시기를 일반 Pop/Unpop에 연결하고 저장한 판본으로 검증한다.</summary>
    void IRawUnpopDisplay.Validate(ReadOnlySpan<byte> slot, uint flags) => Validate(_edition, slot, flags);

    /// <summary>공간 쓰기 전에 미지원 vtable·물리 프레임 범위·판본 불일치를 확인한다.</summary>
    /// <param name="edition">확인할 판본이다.</param>
    /// <param name="bytes">확인할 슬롯 바이트다.</param>
    /// <param name="flags">호출 플래그다.</param>
    public void Validate(OriginalEdition edition, ReadOnlySpan<byte> bytes, uint flags)
    {
        if (edition != _edition)
        {
            throw new ArgumentException("표시 판본 불일치");
        }

        // 실제 메타 88·8c는 공통 frameCheck 부모가 아니며 사전 검증에서 표시 효과를 앞당기지 않는다.
        if (_edition == OriginalEdition.Patch1078 && Read(bytes, 0) == MetaVtable)
        {
            return;
        }

        if (!OriginalUnpopDisplay.Supports(_edition, Read(bytes, 0), flags))
        {
            throw new InvalidOperationException("파생 표시 갱신 미복원");
        }

        if (_edition == OriginalEdition.Patch1078 && Read(bytes, 0) == PatchGeyserVtable && (flags & 0x2000u) != 0)
        {
            uint type = Read(bytes, 10, 1);
            if (type >= (uint)_types.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(bytes), "간헐천 고정 외형 TYPE 범위 오류");
            }

            int frame = _types[(int)type].DefaultFrame;
            // 실제 가상78은 표시 억제보다 먼저 현재 TYPE+128을 읽는다.
            if (!_sink.Suppressed())
            {
                _ = Bounds(type, frame, BitConverter.UInt32BitsToSingle(Read(bytes, 14)), BitConverter.UInt32BitsToSingle(Read(bytes, 18)));
            }

            // 현재 애니메이션 프레임·WORD·extra·그림자는 이 경로의 입력이 아니다.
            return;
        }

        // 마나의 WORD0 경로는 TYPE·SHP·프레임을 읽지 않는다. WORD 비영점도 억제 중이면 bounds에 진입하지 않는다.
        if (_edition == OriginalEdition.Patch1078 && (Read(bytes, 0) == PatchManaVtable || Read(bytes, 0) == BoltVtable))
        {
            if ((Read(bytes, 0) == BoltVtable || Read(bytes, 12, 2) != 0) && !_sink.Suppressed())
            {
                _ = Bounds(Read(bytes, 10, 1), unchecked((int)Read(bytes, 36)), BitConverter.UInt32BitsToSingle(Read(bytes, 14)), BitConverter.UInt32BitsToSingle(Read(bytes, 18)));
            }

            return;
        }

        uint target = Read(bytes, 10, 1);
        if (target >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "표시 타입 번호 범위 오류");
        }

        bool patch = _edition == OriginalEdition.Patch1078;
        int targetFrame = patch ? unchecked((int)Read(bytes, 36)) : (int)Read(bytes, 34, 1);
        // 부분 갱신의 물리 계약만 검증한다. 실제 변경 영역은 Update에서 한 번만 등록한다.
        if ((flags & 0x2000u) != 0 && Read(bytes, 12, 2) == 0 && VortexInsets(_edition, Read(bytes, 0)) is not null)
        {
            if (!_sink.Suppressed())
            {
                _ = Bounds(target, targetFrame, BitConverter.UInt32BitsToSingle(Read(bytes, 14)), BitConverter.UInt32BitsToSingle(Read(bytes, 18)));
            }

            return;
        }

        _ = Read(bytes, patch ? 40 : 35, 1);
        if ((patch && !PatchFrameValid((int)target, targetFrame)) || _sink.Suppressed())
        {
            return;
        }

        SquidDisplayShape shape = _shapes[(int)target];
        if (!shape.Loaded)
        {
            return;
        }

        long last = (long)targetFrame + ((_types[(int)target].Flags1 & Shadow) != 0 ? shape.FrameCount : 0);
        if (targetFrame < 0 || shape.FrameCount < 0 || last < 0 || last >= shape.Frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "표시 물리 프레임 범위 오류");
        }
    }

    /// <summary>공통 가상 메서드의 프레임·extra 비트를 읽어 변경 표에 등록한다.</summary>
    /// <param name="bytes">갱신할 슬롯 바이트다.</param>
    /// <param name="flags">호출 플래그다. 생략하면 0이다.</param>
    public void Update(ReadOnlySpan<byte> bytes, uint flags = 0u)
    {
        // 8c의 004ad670도 현재 88로 꼬리 호출하므로 flags와 무관하게 같은 실제 파생을 선택한다.
        if (_edition == OriginalEdition.Patch1078 && Read(bytes, 0) == MetaVtable)
        {
            _sink.MetaRefresh(bytes);
            return;
        }

        if (_edition == OriginalEdition.Patch1078 && Read(bytes, 0) == PatchGeyserVtable && (flags & 0x2000u) != 0)
        {
            uint type = Read(bytes, 10, 1);
            if (type >= (uint)_types.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(bytes), "간헐천 고정 외형 TYPE 범위 오류");
            }

            // 원문 004acfe0은 현재 raw TYPE의 고정 외형을 반환하며 frameCheck 부모를 호출하지 않는다.
            int frame = _types[(int)type].DefaultFrame;
            if (_sink.Suppressed())
            {
                return;
            }

            DisplayRect rect = Bounds(type, frame, BitConverter.UInt32BitsToSingle(Read(bytes, 14)), BitConverter.UInt32BitsToSingle(Read(bytes, 18)));
            rect = rect with { Top = Coordinate((long)rect.Top + GeyserVerticalInset), Bottom = Coordinate((long)rect.Bottom - GeyserVerticalInset) };
            rect = Clip(rect, _view.Viewport);
            if (Nonempty(rect))
            {
                _sink.Invalidate(rect, 0u);
            }

            // 원문의 유일한 cache 소비자·작성자가 고정한 (0,50,0,50)을 같은 픽셀 순서로 적용한다.
            return;
        }

        uint table = Read(bytes, 0);
        bool patchParent = _edition == OriginalEdition.Patch1078 && table != PatchManaVtable && table != BoltVtable;
        bool vortexPart = patchParent && (flags & 0x2000u) != 0 && Read(bytes, 12, 2) == 0 && VortexInsets(_edition, table) is not null;
        // 신전 부분 갱신도 기본 88 부모를 호출하지 않는다.
        if (patchParent && !vortexPart)
        {
            if (!OriginalUnpopDisplay.Supports(_edition, table, flags))
            {
                throw new InvalidOperationException("파생 표시 갱신 미복원");
            }

            var hooks = new RawSquidDisplayParentHooks();
            // 타입·shape는 현재 같은 배열의 필드 주소를 직접 빌린다.
            hooks.Type = selected =>
            {
                if (selected >= (uint)_types.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(selected), "표시 타입 번호 범위 오류");
                }

                return new RawSquidDisplayTypeView(selected, _types[(int)selected].Flags1, _shapes[(int)selected].FrameCount, _shapes[(int)selected].Loaded, _types[(int)selected].MaxHitPoints);
            };
            hooks.TypeNumber = view => view.Token;
            hooks.FrameError = (number, savedFrame, abstractValue) => _sink.FrameError(number, savedFrame, abstractValue);
            hooks.Assertion = (expression, file, line) => _sink.Assertion(expression, file, line);
            hooks.Mark = (selected, savedFrame, x, y, marker) => Mark(selected, savedFrame, BitConverter.UInt32BitsToSingle(x), BitConverter.UInt32BitsToSingle(y), marker);
            RawSquidDisplayParent.Run(bytes, _sink.Checking(), hooks);
            return;
        }

        Validate(_edition, bytes, flags);
        // 실제 부분 갱신 helper는 표시 억제부터 검사하고 선택 표식·그림자 갱신을 추가하지 않는다.
        if (_edition == OriginalEdition.Patch1078 && (Read(bytes, 0) == PatchManaVtable || Read(bytes, 0) == BoltVtable))
        {
            bool bolt = Read(bytes, 0) == BoltVtable;
            // 원문 45ac90은 WORD와 무관하다. 499240이 음수 여백을 빼므로 오른쪽·아래가 10픽셀 늘어난다.
            if ((!bolt && Read(bytes, 12, 2) == 0) || _sink.Suppressed())
            {
                return;
            }

            DisplayRect boltRect = Bounds(Read(bytes, 10, 1), unchecked((int)Read(bytes, 36)), BitConverter.UInt32BitsToSingle(Read(bytes, 14)), BitConverter.UInt32BitsToSingle(Read(bytes, 18)));
            const int extra = 10;
            boltRect = boltRect with { Right = Coordinate((long)boltRect.Right + extra), Bottom = Coordinate((long)boltRect.Bottom + extra) };
            boltRect = Clip(boltRect, _view.Viewport);
            if (Nonempty(boltRect))
            {
                _sink.Invalidate(boltRect, 0u);
            }

            return;
        }

        bool isPatch = _edition == OriginalEdition.Patch1078;
        uint target = Read(bytes, 10, 1);
        int targetFrame = isPatch ? unchecked((int)Read(bytes, 36)) : (int)Read(bytes, 34, 1);
        // 실제 부분 갱신 helper는 표시 억제부터 검사하고 선택 표식·그림자 갱신을 추가하지 않는다.
        if ((flags & 0x2000u) != 0 && Read(bytes, 12, 2) == 0)
        {
            int[]? insets = VortexInsets(_edition, Read(bytes, 0));
            if (insets is not null)
            {
                if (_sink.Suppressed())
                {
                    return;
                }

                float x = BitConverter.UInt32BitsToSingle(Read(bytes, 14));
                float y = BitConverter.UInt32BitsToSingle(Read(bytes, 18));
                DisplayRect inset = Bounds(target, targetFrame, x, y);
                inset = inset with
                {
                    Left = Coordinate((long)inset.Left + insets[0]),
                    Top = Coordinate((long)inset.Top + insets[1]),
                    Right = Coordinate((long)inset.Right - insets[2]),
                    Bottom = Coordinate((long)inset.Bottom - insets[3]),
                };
                inset = Clip(inset, _view.Viewport);
                if (Nonempty(inset))
                {
                    _sink.Invalidate(inset, 0u);
                }

                return;
            }
        }

        if (isPatch && !PatchFrameValid((int)target, targetFrame))
        {
            return;
        }

        Mark(target, targetFrame, BitConverter.UInt32BitsToSingle(Read(bytes, 14)), BitConverter.UInt32BitsToSingle(Read(bytes, 18)), (Read(bytes, isPatch ? 40 : 35, 1) & 4) != 0 ? Marker : 0u);
    }

    /// <summary>VFX 압축 사각형 대신 별도 Squid 헤더의 pixel 크기와 hotspot을 사용한다.</summary>
    /// <param name="type">타입 번호다.</param>
    /// <param name="frame">프레임 번호다.</param>
    /// <param name="x">가로 좌표다.</param>
    /// <param name="y">세로 좌표다.</param>
    /// <returns>화면 사각형이다.</returns>
    public DisplayRect Bounds(uint type, int frame, float x, float y)
    {
        if (type >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "표시 타입 번호 범위 오류");
        }

        SquidDisplayShape shape = _shapes[(int)type];
        if (!shape.Loaded)
        {
            return new DisplayRect(0, 0, 0, 0);
        }

        if (frame < 0 || frame >= shape.Frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), "표시 프레임 범위 오류");
        }

        SquidDisplayFrame header = shape.Frames[frame];
        int left = Coordinate((long)Project(x, 16, _view.CameraX) - Scale(header.HotspotX, _view.Zoom));
        int top = Coordinate((long)Project(y, 11, _view.CameraY) - Scale(header.HotspotY, _view.Zoom));
        return new DisplayRect(left, top, Coordinate((long)left + Scale(header.Width, _view.Zoom) + 1), Coordinate((long)top + Scale(header.Height, _view.Zoom) + 1));
    }

    /// <summary>패치의 일반 경로에도 남아 있는 frameCheck 조건이다. debug·assert UI는 복원 범위 밖이다.</summary>
    /// <param name="type">타입 번호다.</param>
    /// <param name="frame">프레임 번호다.</param>
    /// <returns>갱신할 수 있으면 참이다.</returns>
    private bool PatchFrameValid(int type, int frame)
    {
        OriginalTypeRecord record = _types[type];
        SquidDisplayShape shape = _shapes[type];
        long count = (long)shape.FrameCount * ((record.Flags1 & DoubleFrames) != 0 ? 2 : 1);
        return frame >= 0 && frame < count && shape.Loaded && record.MaxHitPoints != -0x22222223 && record.MaxHitPoints != -0x32323233;
    }

    /// <summary>기존 4990d0 본문을 현재 인자로 실행한다. 진단 앞의 공간 preflight와 실제 표시 효과를 구별한다.</summary>
    /// <param name="type">타입 번호다.</param>
    /// <param name="frame">프레임 번호다.</param>
    /// <param name="x">가로 좌표다.</param>
    /// <param name="y">세로 좌표다.</param>
    /// <param name="marker">표식 비트다.</param>
    private void Mark(uint type, int frame, float x, float y, uint marker)
    {
        // 실제 표시 억제는 진단 이후 자식 진입에서 평가한다.
        if (_sink.Suppressed())
        {
            return;
        }

        bool patch = _edition == OriginalEdition.Patch1078;
        DisplayRect main = Clip(Bounds(type, frame, x, y), _view.Viewport);
        if (marker != 0)
        {
            if ((long)main.Right - main.Left < 19)
            {
                main = main with { Left = Coordinate((long)main.Left - 9), Right = Coordinate((long)main.Right + 9) };
            }

            main = main with { Top = Coordinate((long)main.Top - (patch ? 15 : 9)) };
            main = main with { Left = Coordinate((long)main.Left - 3), Right = Coordinate((long)main.Right + 3) };
        }

        // clamp 뒤 확장한 범위는 다시 viewport로 자르지 않는다. Dirty·Draw 단계가 화면을 처리한다.
        if (Nonempty(main))
        {
            _sink.Invalidate(main, marker);
        }

        if ((_types[(int)type].Flags1 & Shadow) != 0)
        {
            DisplayRect shadow = Clip(Bounds(type, frame + _shapes[(int)type].FrameCount, x, y), _view.Viewport);
            if (Nonempty(shadow))
            {
                _sink.Invalidate(shadow, marker);
            }
        }
    }

    /// <summary>WORD0인 신전 가상8c는 실제 rune 사각형의 네 inset으로 본그림만 갱신한다. 썬 신전은 기본88의 thunk다.</summary>
    /// <param name="edition">판본이다.</param>
    /// <param name="table">가상 표 값이다.</param>
    /// <returns>네 inset이다. 없으면 null이다.</returns>
    private static int[]? VortexInsets(OriginalEdition edition, uint table)
    {
        bool patch = edition == OriginalEdition.Patch1078;
        if (table == (patch ? 0x5196f0u : 0x501d28u))
        {
            return [12, 29, 12, 23];
        }

        if (table == (patch ? 0x519860u : 0x501de0u))
        {
            return [39, 30, 33, 34];
        }

        if (table == (patch ? 0x519918u : 0x501e98u))
        {
            return [27, 0, 28, 30];
        }

        return null;
    }

    /// <summary>little endian raw 값을 읽으며 부족한 슬롯을 먼저 거부한다.</summary>
    /// <param name="bytes">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        if (offset < 0 || width < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "표시 raw 슬롯 범위 오류");
        }

        // CD의 1바이트 frame 옆 extra를 읽지 않는다.
        uint value = 0u;
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }

        return value;
    }

    /// <summary>새 구현은 큰·손상된 입력의 정수 좌표 넘침을 거부한다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>정수 좌표다.</returns>
    private static int Coordinate(long value)
    {
        if (value < int.MinValue || value > int.MaxValue)
        {
            throw new OverflowException("표시 좌표 넘침");
        }

        return (int)value;
    }

    /// <summary>x*16·y*11+0.5 뒤 절삭이다. float 곱셈으로 먼저 반올림하지 않는다.</summary>
    /// <param name="value">투영할 값이다.</param>
    /// <param name="scale">기울기다.</param>
    /// <param name="camera">뺄 카메라다.</param>
    /// <returns>픽셀 좌표다.</returns>
    private static int Project(float value, double scale, int camera)
    {
        double pixel = (double)value * scale + 0.5;
        if (!double.IsFinite(pixel) || pixel < int.MinValue || pixel > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "표시 좌표 범위 오류");
        }

        return Coordinate((long)pixel - camera);
    }

    /// <summary>원문 imul 하위 DWORD와 산술 우측 이동을 정의된 비트 연산으로 보존한다.</summary>
    /// <param name="value">고를 값이다.</param>
    /// <param name="zoom">Q16 배율이다.</param>
    /// <returns>확대한 값이다.</returns>
    private static int Scale(short value, int zoom)
    {
        uint product = unchecked((uint)((long)value * zoom));
        return unchecked((int)(product >> 16));
    }

    /// <summary>원문은 교집합 계산 대신 각 모서리를 독립 clamp한다.</summary>
    /// <param name="rect">자를 사각형이다.</param>
    /// <param name="viewport">자를 화면이다.</param>
    /// <returns>잘린 사각형이다.</returns>
    private static DisplayRect Clip(DisplayRect rect, DisplayRect viewport)
    {
        return new DisplayRect(Math.Clamp(rect.Left, viewport.Left, viewport.Right), Math.Clamp(rect.Top, viewport.Top, viewport.Bottom), Math.Clamp(rect.Right, viewport.Left, viewport.Right), Math.Clamp(rect.Bottom, viewport.Top, viewport.Bottom));
    }

    /// <summary>양의 면적만 변경 표에 보낸다.</summary>
    /// <param name="rect">검사할 사각형이다.</param>
    /// <returns>면적이 있으면 참이다.</returns>
    private static bool Nonempty(DisplayRect rect)
    {
        return rect.Left < rect.Right && rect.Top < rect.Bottom;
    }
}
