using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 실제 부모가 호출할 때마다 읽는 장치와 최종 배치 경계다
/// (cpppj <c>o/RawAddRandom.h</c>의 <c>AddRandomHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-10에서 추가했다. 카메라·글로벌 난수를 이 부모의 범위·seed로 대체하지 않는다.</remarks>
public sealed class AddRandomHooks
{
    /// <summary>004bcab0→00460d70의 현재 벽시각이다.</summary>
    public Func<double>? WallSeconds { get; set; }

    /// <summary>시각 seed 뒤 읽는 00540c70의 현재 로컬 소유자다.</summary>
    public Func<uint>? Local { get; set; }

    /// <summary>0046f380의 현재 소유자·지역 검색이다.</summary>
    public Func<uint, int, int>? Island { get; set; }

    /// <summary>0040ded0의 지역 난수·사각형·지역·수량·타입·강제다.</summary>
    public Func<NetstormRandom, FortRandomRectangle, int, uint, uint, uint, uint>? Place { get; set; }
}

/// <summary>
/// 0040dfd0의 고정 범위·시각 지역 난수·현재 로컬 섬을 기존 배치 몸체에 조립한다
/// (cpppj <c>o/RawAddRandom</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-10에서 추가했다. 앞선 효과가 실행된 뒤 장치 누락이 발생하면 그 효과를 임의로 되돌리지 않는다.</remarks>
public static class RawAddRandom
{
    /// <summary>
    /// 기본 범위 변환은 원문 00431fc0·0040d900의 고정 1·15를 각각 16배한 값이다.
    /// 반환값은 원문 0040dfd0이 eax로 넘기는 마지막 배치 결과(성공한 마지막 객체 SID, 실패·잘못된 TYPE은 0)이며
    /// 일반 AddRandom 부모는 무시하고 Fort 섬 채우기 0046e630만 이 SID를 읽어 임시 객체를 삭제한다(M6-n에서 반환형 추가).
    /// </summary>
    /// <param name="type">배치할 타입 번호다.</param>
    /// <param name="quantity">성공 목표 수량이다.</param>
    /// <param name="hooks">벽시각·로컬 소유자·섬 검색·배치 경계다.</param>
    /// <returns>마지막 배치 결과 SID다.</returns>
    /// <exception cref="InvalidOperationException">AddRandom 부모의 실제 장치가 연결되지 않았다.</exception>
    /// <remarks>2026-11-11 0.346에서 cpppj HEAD(M6-n 반환형 추가)를 반영했다.</remarks>
    public static uint AddRandomObjects(uint type, uint quantity, AddRandomHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        NetstormRandom random = new(RawFortFinishOwner.TimeSeed(Need(hooks.WallSeconds)()));
        ChunkRect chunk = ChunkMap.DefaultChunkRect();
        FortRandomRectangle rectangle = new(chunk.Left * 16f, chunk.Top * 16f, chunk.Right * 16f, chunk.Bottom * 16f);
        uint owner = Need(hooks.Local)();
        int region = Need(hooks.Island)(owner, 0);
        return Need(hooks.Place)(random, rectangle, region, quantity, type, 0); // 마지막 배치 결과를 원문 반환 레지스터처럼 그대로 돌려준다.
    }

    /// <summary>
    /// 도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">AddRandom 부모의 실제 장치가 연결되지 않았다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("AddRandom 부모의 실제 장치 미연결");
        }
        return function;
    }
}
