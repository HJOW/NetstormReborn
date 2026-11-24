using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Host; // 세션 Root의 실제 외부 장치를 NSInfo 입력과 메타 배치에 공급한다.

// RU-3a의 송신과 음향과 메뉴와 카메라와 입력과 배치 장치를 같은 Root 실장치로 묶는다. 0.986에서 추가됐다.
public sealed class HostNsInfoRootDevicesHooks
{
    /// <summary>같은 실제 12BYTE 상태 패킷을 전송한다. 채우기 뒤 호출된다.</summary>
    public required Action<ReadOnlyMemory<byte>> SendStatus { get; init; }
    /// <summary>같은 실제 추방 확인 음향을 재생한다.</summary>
    public required Action KickoffSound { get; init; }
    /// <summary>같은 실제 guy.wav(0,0,0,0,0) 음향을 재생한다.</summary>
    public required Action BattleGuySound { get; init; }
    /// <summary>같은 실제 참가자 메뉴 공장이다.</summary>
    public required Action<uint, ScreenPoint> ParticipantMenu { get; init; }
    /// <summary>같은 실제 참가자 SID 카메라 중심 이동이다.</summary>
    public required Action<uint> CenterOnSid { get; init; }
    /// <summary>같은 실제 원시 마우스 좌표를 소비 시점에 읽는다.</summary>
    public required Func<ScreenPoint> MousePoint { get; init; }
    /// <summary>처리하지 않은 원래 세 DWORD의 같은 기본 입력 처리다.</summary>
    public required Func<uint, uint, uint, uint> BaseInput { get; init; }
    /// <summary>현재 NSInfo의 실제 메타맵 배치 전체다.</summary>
    public required Action<uint> MetaRootLayout { get; init; }
    /// <summary>현재 Player와 Battle의 실제 메타맵 배치 전체다.</summary>
    public required Action<HudNsInfoChild, uint> MetaChildLayout { get; init; }
    /// <summary>같은 Fancy controlled 메뉴 생성이다.</summary>
    public required Action PlayerMenuBegin { get; init; }
    /// <summary>같은 Fancy 메뉴 부착이다.</summary>
    public required Action PlayerMenuAttach { get; init; }
    /// <summary>같은 실제 메뉴 제목 추가다.</summary>
    public required Action<string> PlayerMenuTitle { get; init; }
    /// <summary>같은 실제 메뉴 항목 작성이다.</summary>
    public required Action<string, uint, uint> PlayerMenuItem { get; init; }
    /// <summary>원문 Sort Game Options By 전체 Fancy 메뉴 자식이다.</summary>
    public required Action<HudBattleInfoNode> BattleSortMenu { get; init; }
    /// <summary>전체 활성 옵션 하위 메뉴와 현재 TYPE 분류와 정렬의 제외 추가 메뉴다.</summary>
    public required Action<HudBattleInfoNode, ReadOnlyMemory<byte>> BattleOptionsMenu { get; init; }
    /// <summary>제외 추가 메뉴다.</summary>
    public required Action<HudBattleInfoNode, ReadOnlyMemory<byte>> BattleExcludeMenu { get; init; }
    /// <summary>선택 옵션 하나의 모든 포함 선택값 메뉴다.</summary>
    public required Action<HudBattleInfoNode, ReadOnlyMemory<byte>, int> BattleOptionMenu { get; init; }
    /// <summary>현재 TYPE 설명을 쓰는 제외 제거 확인 메뉴다.</summary>
    public required Action<uint> BattleRemoveMenu { get; init; }
}

// cpppj RawMenuControls의 OpenMetaParticipant와 Native 입력의 실제 외부 장치를 같은 Root에서 공급한다. 0.986에서 추가됐다.
public static class HostNsInfoRootDevices
{
    /// <summary>같은 실제 12BYTE 패킷을 만들고 채운 뒤 전송한다. 원문 1fh 뒤 부분효과 갈래와 같은 순서다.</summary>
    /// <param name="fill">호출자가 4와 8 오프셋의 두 DWORD를 채운다.</param>
    /// <param name="send">채운 12BYTE를 전송한다.</param>
    public static void SendStatusPacket(Action<Memory<byte>> fill, Action<ReadOnlyMemory<byte>> send)
    {
        ArgumentNullException.ThrowIfNull(fill);
        ArgumentNullException.ThrowIfNull(send);
        byte[] packet = new byte[12];
        fill(packet);
        send(packet);
    }

    /// <summary>RU-3a의 15 장치를 같은 Root 실장치로 묶는다. 하나라도 없으면 미션 진입 전에 거부한다.</summary>
    /// <param name="hooks">같은 Root의 실제 송신과 음향과 메뉴와 카메라와 입력과 배치다.</param>
    /// <param name="baseDevices">이미 연결된 읽기과 번역과 문자열과 변경 장치다.</param>
    /// <returns>창 생성에 그대로 넣는 같은 장치 묶음이다.</returns>
    public static HostNsInfoDevices Build(HostNsInfoRootDevicesHooks hooks, HostNsInfoDevices baseDevices)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(baseDevices);
        return new HostNsInfoDevices
        {
            Read = baseDevices.Read,
            Translate = baseDevices.Translate,
            StoreText = baseDevices.StoreText,
            Text = baseDevices.Text,
            FormatZone = baseDevices.FormatZone,
            Changed = baseDevices.Changed,
            ObjectOwner = baseDevices.ObjectOwner,
            TechWeight = baseDevices.TechWeight,
            RankPicture = baseDevices.RankPicture,
            Describe = baseDevices.Describe,
            Assertion = baseDevices.Assertion,
            MousePoint = hooks.MousePoint,
            ParticipantMenu = hooks.ParticipantMenu,
            CenterOnSid = hooks.CenterOnSid,
            StatusSend = fill => SendStatusPacket(fill, hooks.SendStatus),
            KickoffSound = hooks.KickoffSound,
            BaseInput = hooks.BaseInput,
            MetaRootLayout = hooks.MetaRootLayout,
            MetaChildLayout = hooks.MetaChildLayout,
            PlayerSubscriber = baseDevices.PlayerSubscriber,
            PlayerAuxiliary = baseDevices.PlayerAuxiliary,
            PlayerData = baseDevices.PlayerData,
            PlayerMenuBegin = hooks.PlayerMenuBegin,
            PlayerMenuAttach = hooks.PlayerMenuAttach,
            PlayerMenuTitle = hooks.PlayerMenuTitle,
            PlayerMenuItem = hooks.PlayerMenuItem,
            BattleGuySound = hooks.BattleGuySound,
            BattleSortMenu = hooks.BattleSortMenu,
            BattleOptionsMenu = hooks.BattleOptionsMenu,
            BattleExcludeMenu = hooks.BattleExcludeMenu,
            BattleOptionMenu = hooks.BattleOptionMenu,
            BattleRemoveMenu = hooks.BattleRemoveMenu,
            BattleData = baseDevices.BattleData,
        };
    }
}
