using Netstorm.Core.Original;

namespace Netstorm.Client.Mission;

/// <summary>Guide의 미션 스크립트 생성과 반납을 Native 모드 및 전역 설정과 같은 순서로 수행한다.
/// 근거: cpppj/src/client/Mission.cpp의 CreateGuideMissionScript·ReleaseGuideMissionScript.</summary>
public static class GuideMissionScriptLifetime
{
    /// <summary>스크립트를 만든 뒤 Native 모드와 global.inMission을 공개한다. 생성 실패는 모드를 바꾸지 않는다.</summary>
    /// <param name="configuration">미션 설정 수명보다 오래 살아 있는 설정.</param>
    /// <param name="name">미션 이름.</param>
    /// <param name="missionMode">실제 Native 미션 모드.</param>
    /// <param name="source">선택한 명시 파일 읽기 경계.</param>
    public static MissionScript Create(ConfigInterface configuration, string name, ref bool missionMode, Func<string, byte[]?>? source = null)
    {
        var mission = new MissionScript(configuration, name, source: source);
        try
        {
            missionMode = true;
            configuration.Set("global.inMission", "1");
            return mission;
        }
        catch
        {
            // C++ 공장의 지역 unique_ptr처럼 반환 전 예외에 스크립트를 해제한다. 이미 공개한 모드는 되돌리지 않는다.
            mission.Dispose();
            throw;
        }
    }

    /// <summary>현재 스크립트 등록을 먼저 반납하고 Native 모드와 전역 설정을 차례로 끈다. 현재가 없으면 아무것도 쓰지 않는다.</summary>
    /// <param name="mission">현재 미션 소유 슬롯. 반납 전에 빈 슬롯으로 만든다.</param>
    /// <param name="missionMode">실제 Native 미션 모드.</param>
    /// <param name="configuration">전역 설정과 알림.</param>
    public static void Release(ref MissionScript? mission, ref bool missionMode, ConfigInterface configuration)
    {
        if (mission == null) return;
        MissionScript old = mission;
        mission = null;
        old.Dispose();
        missionMode = false;
        configuration.Set("global.inMission", "0");
    }
}
