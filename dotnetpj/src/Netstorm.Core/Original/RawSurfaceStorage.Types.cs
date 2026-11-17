// 같은 장기 TYPE 목록의 실제 물리 배열 미러를 같은 저장소 수명에 연결한다.
// cpppj/src/client/RawSurfaceWorld.h 의 types 벡터와
// o/SquidReward.h BindCurrentTypes 배열/뷰 별칭 계약을 이식한다.
// 0.572에서 추가됐다. (R4-1 잔여 중 TYPE 배열 미러)
namespace Netstorm.Core.Original;

/// <summary>World 없이 같은 장기 TYPE 목록의 실제 물리 배열 별칭을 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 장기 TYPE 목록의 실제 물리 배열 미러다. 원소 교체 뒤에는 PhysicalTypes() 동기화가 필요하다. 0.572에서 추가됐다.</summary>
    private OriginalTypeRecord[] _physicalTypes = [];

    /// <summary>같은 장기 TYPE 목록을 실제 물리 배열로 빌린다. List 백킹과 구별되는 고정 뷰를 보상·진단이 공유한다. 0.572에서 추가됐다.</summary>
    public OriginalTypeRecord[] PhysicalTypes()
    {
        // 같은 개수·순서로 실제 원소 참조를 배열 미러에 복사한다.
        if (_physicalTypes.Length != Types.Count)
        {
            _physicalTypes = new OriginalTypeRecord[Types.Count];
        }

        // 각 원소 참조를 같은 인덱스에 덮어써 교체된 기록도 다음 조회에 반영한다.
        for (int index = 0; index < Types.Count; index++)
        {
            _physicalTypes[index] = Types[index];
        }

        return _physicalTypes;
    }
}
