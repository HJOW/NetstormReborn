// 거주 구역 주민·장식·지면 테마 사건을 실제 장기 풀·해시·타입·난수와 연결한다.
namespace Netstorm.Core.Original;

// 거주 구역 정기 모듈을 담는 네임스페이스다. 2026-11-19 0.635에서 R4-4 정기 처리용으로 추가했다.

/// <summary>거주 구역 사건이 요구하는 실제 효과 묶음이다.</summary>
/// <remarks>2026-11-19 0.635에서 추가했다. 근거: cpppj <c>o/RawResidenceRegular.h</c>의 <c>ResidenceRegularHooks</c>.</remarks>
public sealed class ResidenceRegularHooks
{
    /// <summary>같은 게임 난수를 직접 소비한다. 2026-11-19 0.635에서 추가했다.</summary>
    public Func<GameRandom>? Random { get; set; }

    /// <summary>주민 변화 뒤 실제 연결 재평가를 호출한다. 2026-11-19 0.635에서 추가했다.</summary>
    public Action<int>? Notify { get; set; }

    /// <summary>같은 실제 프레임 지정기다. 2026-11-19 0.635에서 추가했다.</summary>
    public Action<int, int, uint>? Frame { get; set; }

    /// <summary>거주 구역 아래의 실제 표면 SID를 조회한다. 2026-11-19 0.635에서 추가했다.</summary>
    public Func<float, float, int>? Surface { get; set; }

    /// <summary>가상 소유자 지정의 실제 파생 분배다. 2026-11-19 0.635에서 추가했다.</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>주민 사건의 실제 몸체다. L2 미션 경로에서는 도달하지 않으며 R6에서 잇는다. 2026-11-19 0.635에서 추가했다.</summary>
    public Action<int>? Dude { get; set; }

    /// <summary>신전 종류의 현재 원소 값이다. 2026-11-19 0.635에서 추가했다.</summary>
    public Func<int, int>? Element { get; set; }

    /// <summary>일치 신전 조회 뒤 현재 theammode를 읽는다. 2026-11-19 0.635에서 추가했다.</summary>
    public Func<uint>? ThemeMode { get; set; }

    /// <summary>장식을 탐색하는 실제 타입 번호다. 2026-11-19 0.635에서 추가했다.</summary>
    public Func<uint>? DecorationType { get; set; }

    /// <summary>원본 전역에 저장하는 한 번의 finder 최대 조사 개수다. 2026-11-19 0.635에서 추가했다.</summary>
    public Action<int>? RecordScan { get; set; }
}

/// <summary>PATCH 거주 구역의 현재 조회와 signed 산술·난수 소비 순서를 유지한다.</summary>
/// <remarks>2026-11-19 0.635에서 추가했다. 근거: cpppj <c>o/RawResidenceRegular.cpp</c>의 <c>RunResidenceRegular</c>(원문 0049a4e0)다.</remarks>
public static class RawResidenceRegular
{
    /// <summary>거주 구역 가상표(10.78)다. 이 표의 슬롯만 처리한다. 2026-11-19 0.635에서 추가했다.</summary>
    private const uint ResidenceVtable = 0x0050f540u;

    /// <summary>주민 nibble을 제외한 상위 비트 마스크다. 2026-11-19 0.635에서 추가했다.</summary>
    private const byte ResidentNibbleMask = 0xf0;

    /// <summary>같은 풀의 거주 구역 가상표인지 확인한다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="sid">확인할 슬롯 번호다.</param>
    /// <returns>거주 구역 가상표이면 참이다.</returns>
    public static bool Handles(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            return false;
        }

        return Read(pool.Slot(sid), 0) == ResidenceVtable;
    }

    /// <summary>거주 구역 주민·장식·지면 테마 사건을 실행한다. 사건 2는 R6 주민 생성에서 잇는다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="hash">장식 finder가 읽는 공간 해시다.</param>
    /// <param name="types">타입 번호 순서의 타입 표다. 발자국을 읽는다.</param>
    /// <param name="frames">타입별 프레임 표다. 장식 flags를 읽는다.</param>
    /// <param name="temples">같은 소유자 일치 신전을 찾는 목록이다.</param>
    /// <param name="sid">처리할 거주 구역 부모 번호다.</param>
    /// <param name="event">사건 번호다. 0·1·3만 처리한다.</param>
    /// <param name="count">개수다. 그대로 돌려주지 않고 다음 지연 계산에만 쓴다.</param>
    /// <param name="payload">전달 값이다. 사건 0·1의 재예약으로 바뀐다.</param>
    /// <param name="hooks">실제 효과 묶음이다.</param>
    /// <returns>다음 예약 지연이다.</returns>
    /// <exception cref="ArgumentException">풀·타입·가상 표 불일치다.</exception>
    /// <exception cref="InvalidOperationException">선택된 실제 장치가 미연결이다.</exception>
    public static float Run(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, RawPostPopList temples, int sid, uint @event, uint count, float payload, ResidenceRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(temples);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("ResidenceRegular는 PATCH10.78 본문이다", nameof(pool));
        }

        if (!Handles(pool, sid))
        {
            throw new ArgumentException("거주 구역 정기 가상 표·타입 미복원", nameof(sid));
        }

        if (@event == 0)
        {
            Span<byte> raw = pool.RawSlot(sid);
            byte low = raw[12];
            raw[12] = unchecked((byte)((low & ResidentNibbleMask) | (((low & 15) == 0) ? 1 : 0)));
            ResidenceNeed(hooks.Notify)(sid);
            SetEven(pool, sid, hooks);
            return (float)((double)ResidenceNeed(hooks.Random)().Next(65536) * (20.0 / 65536.0) + 15.0);
        }

        if (@event == 1)
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            double x = BitConverter.UInt32BitsToSingle(Read(raw, 14));
            double y = BitConverter.UInt32BitsToSingle(Read(raw, 18));
            GameRandom random = ResidenceNeed(hooks.Random)();
            int searchX = Add(Truncate(x - 48.0), unchecked((int)random.Next(unchecked((uint)Truncate(x + 32.0) - unchecked((uint)Truncate(x - 48.0))))));
            int searchY = Add(Truncate(y - 32.0), unchecked((int)random.Next(unchecked((uint)Truncate(y + 32.0) - unchecked((uint)Truncate(y - 32.0))))));
            searchX = Math.Clamp(searchX, 10, 246);
            searchY = Math.Clamp(searchY, 10, 246);
            int bottom = Truncate(Math.Min((double)(float)(y + 32.0), 246.0));
            var finder = new RawSquidFinder(pool, hash, types);
            int candidate = finder.Begin(new SquidSearchArea(searchX - 1, searchY - 3, searchX + 1, bottom));
            bool changed = false;
            int benchmarked = 0;
            // 첫 변경 뒤에도 실제 Next를 한 번 수행한다.
            while (candidate != 0 && !changed)
            {
                benchmarked++;
                if (pool.Slot(candidate)[RawSidPool.TypeOffset] == ResidenceNeed(hooks.DecorationType)())
                {
                    IReadOnlyList<RiftFrameCode> codes = frames[pool.Slot(candidate)[RawSidPool.TypeOffset]].Codes;
                    int frame = unchecked((int)Read(pool.Slot(candidate), 36));
                    if (frame < 0 || frame >= codes.Count)
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), "거주 구역 현재 장식 frame 범위");
                    }

                    byte flags = codes[frame].Flags;
                    if ((flags & 8) != 0)
                    {
                        ResidenceNeed(hooks.Frame)(candidate, Add(unchecked((int)Read(pool.Slot(candidate), 36)), 1), 0);
                        changed = true;
                    }
                    else if ((flags & 16) != 0)
                    {
                        ResidenceNeed(hooks.Frame)(candidate, Add(unchecked((int)Read(pool.Slot(candidate), 36)), -1), 0);
                        changed = true;
                    }
                }

                candidate = finder.Next();
            }

            ResidenceNeed(hooks.RecordScan)(benchmarked);
            return 1.0f;
        }

        if (@event == 2)
        {
            ResidenceNeed(hooks.Dude)(sid);
            return payload;
        }

        if (@event == 3)
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            int support = ResidenceNeed(hooks.Surface)(BitConverter.UInt32BitsToSingle(Read(raw, 14)), BitConverter.UInt32BitsToSingle(Read(raw, 18)));
            if (support != 0)
            {
                ResidenceNeed(hooks.SetOwner)(sid, pool.Slot(support)[34]);
            }

            int theme = 0;
            uint owner = pool.Slot(sid)[34];
            // 원문이 캡처한 목록/count와 owner에서 첫 일치 신전만 선택한다.
            foreach (uint value in temples.Items())
            {
                int temple = unchecked((int)value);
                if (pool.Slot(temple)[34] != owner)
                {
                    continue;
                }

                int element = ResidenceNeed(hooks.Element)(temple);
                uint enabled = ResidenceNeed(hooks.ThemeMode)();
                if (enabled != 0)
                {
                    theme = element == 2 ? 1 : element == 0 ? 2 : element == 1 ? 3 : 0;
                }

                break;
            }

            ResidenceNeed(hooks.Frame)(sid, Add(unchecked((int)Read(pool.Slot(sid), 42)), theme * 6), 0);
            SetEven(pool, sid, hooks);
            return 0.1f;
        }

        return payload;
    }

    /// <summary>현재 프레임의 짝수 내림 뒤 현재 주민 nibble을 더해 지정한다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="sid">쓸 슬롯 번호다.</param>
    /// <param name="hooks">실제 프레임 지정 묶음이다.</param>
    private static void SetEven(RawSidPool pool, int sid, ResidenceRegularHooks hooks)
    {
        int frame = unchecked((int)Read(pool.Slot(sid), 36));
        ResidenceNeed(hooks.Frame)(sid, Add(frame / 2 * 2, pool.Slot(sid)[12] & 15), 0);
    }

    /// <summary>x87 ftol의 signed64 범위 low DWORD다. 비유한·범위 밖 입력은 거부한다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    private static int Truncate(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "거주 구역 좌표 CRT 범위 오류");
        }

        return unchecked((int)(long)value);
    }

    /// <summary>원본 signed 덧셈의 32비트 넘침을 보존한다.</summary>
    private static int Add(int value, int delta) => unchecked((int)(unchecked((uint)value) + unchecked((uint)delta)));

    /// <summary>정렬되지 않은 little endian 값을 현재 슬롯에서 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 현재 필드의 지정 바이트만 조합한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }

        return value;
    }

    /// <summary>도달한 장치만 요구하며 콜백 중 원본 callable의 수명을 보존한다.</summary>
    private static T ResidenceNeed<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("거주 구역의 선택된 실제 본문 미연결");
}
