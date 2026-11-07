// 표면의 기존 훅을 합성한 미션 runtime 경계와 전송 대리자를 공유한다.
namespace Netstorm.Core.Original;

// 월드 런타임 훅 모듈을 담는 네임스페이스다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.

// Form Pop 전송이다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.
public delegate void FormPopTransmit(
    /// <summary>Pop할 Form 번호다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    int form,
    /// <summary>Pop 플래그다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    uint flags);

// 일반 Squid writer가 완성한 현재 패킷을 같은 실제 방송 분배기로 보낸다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.
public delegate void BattlePacketSend(
    /// <summary>보낼 패킷 바이트다. 원문 span을 배열 복사로 옮긴다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    byte[] packet);

// 자산 삭제는 `RawSquidFormDispatch.cs`의 동명 대리자를 빌린다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.

// 표면의 기존 훅을 합성한 미션 runtime 경계다. 새 월드 생성 뒤 첫 객체 생성·적재 전에 한 번 연결한다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.
public sealed class RawWorldRuntimeHooks
{
    /// <summary>타입·셰이프·팔레트 자산 또는 해당 자산의 처리 경계다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public SquidDestroyHooks? Assets { get; set; }

    /// <summary>객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public SquidAssetDestroy? Destroy { get; set; }

    /// <summary>주기 동작을 수행하는 Regular 프로세스 연결이다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public RegularHandler? Regular { get; set; }

    /// <summary>Form 데이터 작성과 송신을 연결하는 전송 경계다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public ProcessFormTransmission? Transmit { get; set; }

    /// <summary>지정 이동 유닛의 추락 처리를 실행하는 콜백이다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public Func<int, bool>? WalkerFall { get; set; }

    /// <summary>현재 로딩 중첩 깊이를 외부 상태에 전달하는 콜백이다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public Action<uint>? LoadingDepth { get; set; }

    /// <summary>실제 DependForm 서버 Pop의 현재 raw Form·flags 전송 요청 경계다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public FormPopTransmit? FormTransmit { get; set; }

    /// <summary>일반 Squid writer가 완성한 현재 패킷을 같은 실제 방송 분배기로 보낸다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public BattlePacketSend? SquidPacket { get; set; }

    /// <summary>원문 Player+50 bit1의 대상별 접속 관문이다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public Func<int, bool>? SquidConnected { get; set; }

    /// <summary>같은 현재 대상의 연결 객체로만 패킷을 보낸다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public Action<int, byte[]>? SquidSend { get; set; }

    /// <summary>watcher의 실제 가상 통지다. 누락 시 기존 ProcessForm 통지를 사용한다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public DependFormNotice? DependencyNotice { get; set; }

    /// <summary>실제 세션의 다리 파편·위치 소리 소비자이며 runtime 설치 전에 필수로 공급한다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    public Action<BridgeLifecycleEvent>? BridgeEffect { get; set; }

    /// <summary>현재 런타임 묶음의 얕은 사본을 만든다. 참조·대리자는 공유한다. 2026-11-07 F2d 월드 런타임 훅 이식에서 추가됐다.</summary>
    /// <returns>복사된 런타임 묶음이다.</returns>
    public RawWorldRuntimeHooks Clone() => new()
    {
        Assets = Assets,
        Destroy = Destroy,
        Regular = Regular,
        Transmit = Transmit,
        WalkerFall = WalkerFall,
        LoadingDepth = LoadingDepth,
        FormTransmit = FormTransmit,
        SquidPacket = SquidPacket,
        SquidConnected = SquidConnected,
        SquidSend = SquidSend,
        DependencyNotice = DependencyNotice,
        BridgeEffect = BridgeEffect,
    };
}
