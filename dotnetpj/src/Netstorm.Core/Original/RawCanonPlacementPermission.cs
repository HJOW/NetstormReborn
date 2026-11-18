// 원본 후보 권한 함수의 그래프/소유 관계 접두와 DWORD 작업장 조회 반환값을 복원한다.
namespace Netstorm.Core.Original;

/// <summary>
/// 일반 타입·패턴 배치 허가 처리 중 조회·갱신하는 상태 묶음이다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 소유 관계 판정에 쓴다.
/// 근거: cpppj/src/o/RawCanonPlacementPermission.h.
/// </summary>
/// <remarks>2026-11-18 0.609에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. R6-3 I 영역의 3단위 중 첫째다.</remarks>
public sealed class CanonPlacementPermissionState
{
    /// <summary>그래프 조회가 가능한 상태다. 0이면 후보 검사를 거부한다.</summary>
    public uint GraphReady { get; set; }

    /// <summary>편집 모드다. 켜져 있으면 소유 관계 없이 허용한다.</summary>
    public uint Editor { get; set; }

    /// <summary>방향 동맹을 적용한다. 꺼져 있으면 동일 소유자만 허용한다.</summary>
    public uint UseAlliances { get; set; }

    /// <summary>다른 소유자의 배치를 허용한다. 주변 검사의 마지막에 읽는다.</summary>
    public uint AllowOtherOwners { get; set; }

    /// <summary>현재 소유자×9+요청자 순서인 원본 DWORD 관계 표다.</summary>
    public uint[] Alliances { get; } = new uint[81];
}

/// <summary>
/// 원본 후보 권한 함수의 그래프/소유 관계 접두와 DWORD 작업장 조회 반환값이다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 소유 관계 판정에 쓴다.
/// 근거: cpppj/src/o/RawCanonPlacementPermission.h/.cpp, 원본 00462cb0·004629e0.
/// </summary>
/// <remarks>2026-11-18 0.609에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. R6-3 I 영역의 3단위 중 첫째다.</remarks>
public sealed class RawCanonPlacementPermission
{
    /// <summary>원본이 읽는 슬롯의 타입·소유자 바이트 위치다.</summary>
    private const int TypeOffset = 10;

    /// <summary>원본 좌표를 읽는 X DWORD 위치다.</summary>
    private const int XOffset = 14;

    /// <summary>원본 좌표를 읽는 Y DWORD 위치다.</summary>
    private const int YOffset = 18;

    /// <summary>패치판 소유자 바이트 위치다.</summary>
    private const int PatchOwnerOffset = 34;

    /// <summary>CD판 소유자 바이트 위치다.</summary>
    private const int CdOwnerOffset = 32;

    /// <summary>같은 풀의 슬롯을 읽는 SID 풀이다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>처리 중 읽는 상태다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly CanonPlacementPermissionState _state;

    /// <summary>플레이어·타입·좌표에 맞는 실제 배치 기준 객체를 찾는 콜백이다.</summary>
    private readonly Func<uint, uint, float, float, uint> _playerAnchor;

    /// <summary>
    /// Player 작업장/그래프 locator를 필수 경계로 받는다. 없으면 거부한다.
    /// </summary>
    /// <param name="pool">같은 풀의 슬롯을 읽는다.</param>
    /// <param name="state">처리 중 읽는 상태다.</param>
    /// <param name="playerAnchor">플레이어·타입·좌표에 맞는 실제 배치 기준 객체를 찾는 콜백이다.</param>
    public RawCanonPlacementPermission(RawSidPool pool, CanonPlacementPermissionState state, Func<uint, uint, float, float, uint> playerAnchor)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(playerAnchor);
        _pool = pool;
        _state = state;
        _playerAnchor = playerAnchor;
    }

    /// <summary>
    /// 00462cb0 / CD 0045ba40: 그래프 비활성→요청자 0→현재 raw 소유 관계 순서의 DWORD 반환이다.
    /// </summary>
    /// <param name="candidate">조회할 후보 SID다.</param>
    /// <param name="owner">요청 소유자다.</param>
    /// <returns>배치 기준 객체 값이다. 0이면 거부다.</returns>
    public uint Candidate(int candidate, uint owner)
    {
        if (_state.GraphReady == 0)
        {
            return 0;
        }
        if (owner == 0)
        {
            return 1;
        }
        ReadOnlySpan<byte> raw = _pool.Slot(candidate);
        byte currentOwner = raw[_pool.Edition == OriginalEdition.Patch1078 ? PatchOwnerOffset : CdOwnerOffset];
        // 현재 raw 소유 관계와 요청자를 구별한다. 중립이 아니고 관계가 없으면 거부한다.
        if (!Related(currentOwner, owner) && currentOwner != 0 && _state.AllowOtherOwners == 0)
        {
            return 0;
        }
        return _playerAnchor(owner, raw[TypeOffset], ReadCoordinate(raw, XOffset), ReadCoordinate(raw, YOffset));
    }

    /// <summary>
    /// 주변 순회의 바깥 관계 검사를 먼저 적용한다. 중립 소유자에도 적용하며 다른 소유자 허용은 마지막에 읽는다.
    /// </summary>
    /// <param name="candidate">조회할 후보 SID다.</param>
    /// <param name="owner">요청 소유자다.</param>
    /// <returns>통과하면 0이 아닌 값이다.</returns>
    public uint Surrounding(int candidate, uint owner)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(candidate);
        byte currentOwner = raw[_pool.Edition == OriginalEdition.Patch1078 ? PatchOwnerOffset : CdOwnerOffset];
        // 중립도 바깥 검사를 적용하고 다른 소유자 허용은 마지막에 읽는다.
        if (!Related(currentOwner, owner) && _state.AllowOtherOwners == 0)
        {
            return 0;
        }
        return Candidate(candidate, owner);
    }

    /// <summary>
    /// 004629e0: 편집기·동일 소유자·방향 동맹을 검사한다. 중립·다른 소유자 허용은 여기다.
    /// </summary>
    /// <param name="currentOwner">현재 소유자다.</param>
    /// <param name="owner">요청 소유자다.</param>
    /// <returns>관계가 있으면 참이다.</returns>
    public bool Related(uint currentOwner, uint owner)
    {
        if (_state.Editor != 0 || currentOwner == owner)
        {
            return true;
        }
        if (_state.UseAlliances == 0)
        {
            return false;
        }
        return Alliance(currentOwner, owner);
    }

    /// <summary>
    /// 최종 소유자 거부는 원본처럼 관계 표를 직접 읽으며 편집기·동일 owner·활성 조건을 붙이지 않는다.
    /// </summary>
    /// <param name="currentOwner">현재 소유자다.</param>
    /// <param name="owner">요청 소유자다.</param>
    /// <returns>관계 표 값이 0이 아니면 참이다.</returns>
    public bool Alliance(uint currentOwner, uint owner)
    {
        uint index = currentOwner * 9u + owner;
        if (index >= (uint)_state.Alliances.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(currentOwner), "배치 권한 관계 표 범위 오류");
        }
        return _state.Alliances[(int)index] != 0;
    }

    /// <summary>같은 raw 풀을 사용하는 지형 경계인지 확인한다.</summary>
    /// <returns>이 모듈이 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 비정렬 원본 float 비트값을 DWORD로 읽는다.
    /// 낮은 바이트부터 조립하며 NaN·부호 있는 0을 산술 변환하지 않는다.
    /// </summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>비트 그대로의 float 값이다.</returns>
    private static float ReadCoordinate(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 낮은 바이트부터 조립하는 반복문이다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }
        return BitConverter.UInt32BitsToSingle(value);
    }

    /// <summary>
    /// 원본 MayPlace의 요청 owner 하위 바이트를 부호 확장하고 후보 권한만 실제 helper에 연결한다.
    /// </summary>
    /// <param name="pool">같은 풀의 실제 Player 조회다. 다른 풀의 몸체와 섞으면 거부한다.</param>
    /// <param name="permission">같은 풀의 소유 관계 몸체다.</param>
    /// <param name="hooks">물을 훅 묶음이다. 후보 권한만 실제 구현으로 바꾼다.</param>
    /// <returns>후보 권한이 연결된 훅 묶음이다.</returns>
    public static CanonPlacementTerrainHooks MakeCanonPermissionTerrainHooks(RawSidPool pool, RawCanonPlacementPermission permission, CanonPlacementTerrainHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(permission);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, permission.Pool))
        {
            throw new ArgumentException("배치 권한/지형 SID 풀 불일치", nameof(pool));
        }
        CanonPlacementTerrainHooks result = new()
        {
            CandidatePermission = hooks.CandidatePermission,
            ShapePermission = hooks.ShapePermission,
            FinishRelations = hooks.FinishRelations,
        };
        result.CandidatePermission = (query, sid) =>
        {
            // 요청 owner 하위 바이트를 부호 확장한 signed BYTE 계약으로 검사한다.
            uint owner = unchecked((uint)(int)(sbyte)(byte)query.Owner);
            return permission.Candidate(sid, owner) != 0;
        };
        return result;
    }
}
