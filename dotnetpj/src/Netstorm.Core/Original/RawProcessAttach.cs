// ProcessForm의 파생 접두를 공통 Form 부착과 구별한다.
namespace Netstorm.Core.Original;

// 프로세스 부착 모듈을 담는 네임스페이스다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.

// 현재 서버와 진단·선택 장치를 원문 소비 순서에 제공한다. 선택되지 않은 직렬화·송신 장치는 요구하지 않는다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.
public sealed class ProcessAttachHooks
{
    /// <summary>부모 변경 진단 뒤 현재 서버 전역을 읽는다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    public Func<bool>? Server { get; set; }

    /// <summary>서버이면 로컬 플래그에서도 현재 PID의 프로세스를 먼저 찾는다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    public Action<int>? Lookup { get; set; }

    /// <summary>저장한 프로세스의 실제 writer와 최초 부모 변경 판정을 사용한다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    public Func<bool, int>? WriteData { get; set; }

    /// <summary>양수 길이 진단 정상 복귀 뒤에만 선택된다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    public Action<int>? Transmit { get; set; }

    /// <summary>파생 접두 뒤 현재 공통 Form 부모로 넘긴다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    public Action<int, int, uint>? Attach { get; set; }

    /// <summary>진단 정상 복귀와 명시 중단을 구별한다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 훅 묶음의 얕은 사본을 만든다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    /// <returns>복사된 훅 묶음이다.</returns>
    public ProcessAttachHooks Clone() => new()
    {
        Server = Server,
        Lookup = Lookup,
        WriteData = WriteData,
        Transmit = Transmit,
        Attach = Attach,
        Assertion = Assertion,
    };
}

// 실제 프로세스·직렬화 저장소는 외부 장치가 소유한다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.
public static class RawProcessAttach
{
    /// <summary>송신 버퍼 상한(0x7ff)이다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    public const int TransmitBufferLimit = 0x7ff;

    /// <summary>콜백 자체 교체에도 현재 호출이 복귀할 때까지 선택한 callable을 보관한다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    /// <param name="device">쓸 장치다.</param>
    private static T Need<T>(T? device) where T : class
    {
        if (device is null)
        {
            throw new InvalidOperationException("프로세스 부착의 선택된 실제 장치 미연결");
        }

        return device;
    }

    /// <summary>파생 접두의 진단·선택·공통 부착 순서다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="form">고를 Form SID다.</param>
    /// <param name="parent">부를 부모 SID다.</param>
    /// <param name="flags">원문 플래그다.</param>
    /// <param name="hooks">선택 장치 묶음이다.</param>
    public static void Attach(RawSidPool pool, int form, int parent, uint flags, ProcessAttachHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        bool patch = pool.Edition == OriginalEdition.Patch1078;
        ReadOnlySpan<byte> raw = pool.Slot(form);
        // 첫 진단 전에 읽은 부모 변경 판정을 유지한다
        bool changed = (ushort)(raw[16] | (raw[17] << 8)) != parent;
        void Diagnose(string expression, uint patchLine, uint cdLine) =>
            Need(hooks.Assertion)(expression, patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp", patch ? patchLine : cdLine);
        if (changed && (flags & 0x80u) == 0u)
        {
            Diagnose("popFlags & pfPROCESS_DEBUG", 0xa77u, 0xa2eu);
        }

        if (Need(hooks.Server)())
        {
            Need(hooks.Lookup)(form);
            if ((flags & 0x50u) == 0u)
            {
                int size = Need(hooks.WriteData)(changed);
                if (size == 0)
                {
                    Diagnose("transmitSize!=0", 0xa7fu, 0xa36u);
                }

                if (size > 0)
                {
                    if (size > TransmitBufferLimit)
                    {
                        Diagnose("transmitSize < TRANSMIT_BUFFER_SIZE", 0xa81u, 0xa38u);
                    }

                    Need(hooks.Transmit)(size);
                }
            }

            flags |= 0x40u;
        }

        Need(hooks.Attach)(form, parent, flags);
    }
}
