// Native 장기 자원 장치와 같은 실제 세션 본체를 연결한다.
// cpppj/src/client/ClientMain.cpp 468~477행의 BindResourceDevices 일곱 항목과
// cpppj/src/client/RawCampaignSession.cpp 4690~4712행의 여섯 본문을 이식한다.
// 0.901에서 RU-1 실제 owner 조립용으로 추가됐다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>자원 후처리가 선택 위치에서 호출하는 실제 세션 본체다.</summary>
public sealed partial class HostSession
{
    /// <summary>원문 cc가 실제 선택된 위치만 현재 전체 owner를 요구한다. 0.901에서 추가됐다.</summary>
    /// <param name="sid">확인할 객체 번호다.</param>
    public void CheckCarrierPostPop(int sid)
    {
        // 기존 직접 Carrier 후처리와 같은 가상 확인 종단이다.
        // 근거: cpppj RawCampaignSession.cpp 4694행.
        PriestWorld.CheckCarrier(sid);
    }

    /// <summary>실제 피해 사건의 현재 큰 세계 종단이다. 0.901에서 추가됐다.</summary>
    /// <param name="sid">피해 객체 번호다.</param>
    /// <param name="event">예약 사건 번호다.</param>
    /// <param name="count">개수다.</param>
    /// <param name="payload">실수 인자다.</param>
    public float HandleDamageableResourceRegular(int sid, uint @event, uint count, float payload)
    {
        // 장기 피해·지면 부모는 R6 피해 결선에서 잇는다. 미설치를 성공으로 꾸미지 않는다.
        throw new InvalidOperationException("호스트 피해 자원 정기의 실제 큰 세계 종단 미연결(R6)");
    }

    /// <summary>현재 Form 관문을 통과한 건설 재시도만 같은 부모에 전달한다. 0.901에서 추가됐다.</summary>
    /// <param name="sid">건설을 시작할 부모 번호다.</param>
    public void StartResourceConstruction(int sid)
    {
        // 실제 현재 Form 조회 뒤 선택한 건설 Start(parent, 0)만 같은 현재 상태로 실행한다.
        // 근거: cpppj RawCampaignSession.cpp 4691행. 건설 본체는 R6에서 잇는다.
        throw new InvalidOperationException("호스트 자원 건설 시작의 실제 건설 본체 미연결(R6)");
    }

    /// <summary>선택된 실제 가상 삭제의 수락/거부를 그대로 반환한다. 0.901에서 추가됐다.</summary>
    /// <param name="sid">삭제할 객체 번호다.</param>
    /// <param name="flags">삭제 플래그다.</param>
    public bool DestroyResourceObject(int sid, uint flags)
    {
        // 근거: cpppj RawCampaignSession.cpp 4692행의 impl_->world.Destroy.
        // 실제 가상10의 bool과 부분 효과를 같은 호출자에게 돌려준다.
        return DestroyForState(sid, flags);
    }

    /// <summary>기존 최초 Pop/삭제와 동일한 실제 AI 목록 본문이다. 0.901에서 추가됐다.</summary>
    /// <param name="token">현재 Player가 가리키는 본체다.</param>
    /// <param name="sid">간헐천 SID다.</param>
    /// <param name="operation">원본 state다.</param>
    public void NotifyGeyserAi(uint token, uint sid, uint operation)
    {
        // 근거: cpppj RawCampaignSession.cpp 4693행의 impl_->aiWorld->Geyser.
        AiWorld.Geyser(token, sid, operation);
    }

    /// <summary>Client가 pin한 현재 전체 Runtime의 기존 본체만 실행한다. 0.901에서 추가됐다.</summary>
    /// <param name="sid">부모 번호다.</param>
    /// <param name="event">사건 번호다.</param>
    /// <param name="count">개수다.</param>
    /// <param name="payload">실수 인자다.</param>
    public float HandleCarrierResourceRegular(int sid, uint @event, uint count, float payload)
    {
        // 근거: cpppj RawCampaignSession.cpp 4712행. 실제 Carrier Regular 설치 전 호출은 거부한다.
        throw new InvalidOperationException("호스트 운반체 자원 정기의 실제 Carrier 본체 미연결(R6)");
    }
}
