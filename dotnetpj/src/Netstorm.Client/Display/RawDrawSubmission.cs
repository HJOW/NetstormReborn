// 원본 Renderer의 한 유효 dirty 사각형 안에서 표시 목록 병합과 kind 0~4 분배를 복원한다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Display;

/// <summary>요청 범위 안의 출력 그룹을 조회하는 콜백이다. 배경 덮음은 참조로 돌려준다.</summary>
/// <param name="view">수집에 쓰는 월드 사각형이다.</param>
/// <param name="rowLimit">묶음 각 행의 count&lt;limit assert에 쓰는 판본별 값이다.</param>
/// <param name="covered">수집한 항목이 배경 영역을 덮는지. false면 원본은 월드 배경을 다시 그린다.</param>
/// <returns>현재 수집의 묶음 보기다.</returns>
public delegate IReadOnlyList<RawTileGroupView> DrawGroupsHook(RenderWorldRect view, int rowLimit, ref bool covered);

/// <summary>수집 순서는 UI→건설 표시→묶음→일반 자산/효과→미리보기다. 빈 콜백은 아직 연결하지 않은 수집기다.</summary>
public sealed class RawDrawCollectionHooks
{
    /// <summary>UI 출력 항목을 수집하는 콜백이다.</summary>
    public Func<ScreenRect, int, IReadOnlyList<RenderCollectionEntry>>? Ui { get; set; }
    /// <summary>건설 출력 항목을 수집하는 콜백이다.</summary>
    public Func<RenderWorldRect, int, IReadOnlyList<RenderCollectionEntry>>? Construction { get; set; }
    /// <summary>요청 범위 안의 출력 그룹을 조회하는 콜백이다.</summary>
    public DrawGroupsHook? Groups { get; set; }
    /// <summary>요청 범위 안의 객체 출력 항목을 수집하는 콜백이다.</summary>
    public Func<RenderWorldRect, int, IReadOnlyList<RenderCollectionEntry>>? Objects { get; set; }
    /// <summary>배치 미리보기의 출력 항목을 수집하는 콜백이다.</summary>
    public Func<RenderWorldRect, int, IReadOnlyList<RenderCollectionEntry>>? Preview { get; set; }
}

/// <summary>이미 환산한 두 월드 영역을 구분해 전달한다. 미리보기는 클립 전 영역을 받는다.</summary>
/// <param name="Screen">화면 좌표의 사각형이다. right·bottom은 포함하지 않는다.</param>
/// <param name="Visible">표시 대상 또는 표시 허용 상태다.</param>
/// <param name="Unclipped">클리핑을 적용하기 전의 출력 사각형이다.</param>
/// <param name="DirtyFlags">갱신할 출력 계층을 선택하는 플래그다. bit 2는 UI만 수집한다.</param>
/// <param name="GroupRowLimit">수집할 그룹 행의 최대 개수다.</param>
public readonly record struct RawDrawCollectionRequest(ScreenRect Screen, RenderWorldRect Visible, RenderWorldRect Unclipped, uint DirtyFlags, int GroupRowLimit);

/// <summary>묶음 수집기의 배경 덮음 관찰을 함께 반환한다. false면 원본은 월드 배경을 다시 그린다.</summary>
/// <param name="Count">현재 항목 수다.</param>
/// <param name="BackgroundCovered">수집한 항목이 배경 영역을 덮는지.</param>
/// <param name="Entries">순차 병합한 표시 항목이다.</param>
public readonly record struct RawDrawCollectionResult(int Count, bool BackgroundCovered, IReadOnlyList<RenderCollectionEntry> Entries);

/// <summary>수집된 스프라이트 항목을 출력하는 콜백이다.</summary>
public sealed class RawDrawDispatchHooks
{
    /// <summary>수집한 스프라이트 항목을 출력하는 콜백이다.</summary>
    public Action<RenderCollectionEntry>? Sprite { get; set; }
    /// <summary>토큰에 대응하는 출력 그룹을 조회하는 콜백이다.</summary>
    public Func<uint, RawTileGroupView>? Group { get; set; }
    /// <summary>UI 토큰을 출력하는 콜백이다.</summary>
    public Action<uint>? Ui { get; set; }
    /// <summary>건설 토큰을 출력하는 콜백이다.</summary>
    public Action<uint>? Construction { get; set; }
    /// <summary>미리보기 항목 전체를 출력하는 콜백이다.</summary>
    public Action<RenderCollectionEntry>? Preview { get; set; }
    /// <summary>상위 항목마다 한 번 호출하는 디버그 후속이다.</summary>
    public Action? AfterEach { get; set; }
}

/// <summary>공통 수집과 종류 분배의 묶음 슬롯을 같은 실제 수집기에 연결한다.</summary>
/// <remarks>cpppj client/DrawSubmission.cpp의 10.78 004994b0 ↔ CD 004575e0 대응이다. 좌표 환산/dirty 순회/배경/장치 잠금은 호출자의 책임이다. UI 내부는 GumpCollection으로 연결할 수 있다. 2026-11-26 1.065에서 R5-2c용으로 추가했다.</remarks>
public static class RawDrawSubmission
{
    /// <summary>원본 공통 작업 배열은 16바이트 표시 항목 5,000개다.</summary>
    public const int Capacity = 5000;

    /// <summary>연결된 종류의 그리기 경계가 빠졌다면 빈 함수를 호출하기 전에 계약 오류를 알린다.</summary>
    /// <typeparam name="T">연결된 장치의 형태다.</typeparam>
    /// <param name="callback">요구할 실제 장치다.</param>
    /// <returns>같은 장치다.</returns>
    private static T Required<T>(T? callback) where T : Delegate =>
        callback ?? throw new InvalidOperationException("공통 표시 분배 콜백 누락");

    /// <summary>용량을 5,000 이하로 제한해 순차 병합한다. 묶음의 kind만 1로 바꾸고 tail은 0으로 둔다.</summary>
    /// <param name="request">수집 요청이다.</param>
    /// <param name="hooks">연결된 수집기다.</param>
    /// <returns>병합 결과다.</returns>
    public static RawDrawCollectionResult CollectDrawEntries(RawDrawCollectionRequest request, RawDrawCollectionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        var work = new List<RenderCollectionEntry>(Capacity);
        // 빈 수집기도 건너뛰되 연결된 수집기는 남은 용량 0에서 호출되는 원문 순서를 유지한다
        void Append(IReadOnlyList<RenderCollectionEntry>? items, int limit)
        {
            if (items is null)
            {
                return;
            }
            if (items.Count > limit)
            {
                throw new InvalidOperationException("표시 수집 반환 개수가 남음을 넘었습니다");
            }
            work.AddRange(items);
        }
        int Remaining() => Capacity - work.Count;
        if (hooks.Ui is not null)
        {
            Append(hooks.Ui(request.Screen, Remaining()), Remaining());
        }
        if ((request.DirtyFlags & 4U) != 0)
        {
            return new(work.Count, false, work);
        }
        if (hooks.Construction is not null)
        {
            Append(hooks.Construction(request.Visible, Remaining()), Remaining());
        }
        bool covered = false;
        if (hooks.Groups is not null)
        {
            IReadOnlyList<RawTileGroupView> groups = hooks.Groups(request.Visible, request.GroupRowLimit, ref covered);
            if (groups.Count > Remaining())
            {
                throw new InvalidOperationException("표시 묶음 용량 초과");
            }
            // 묶음의 첫 키와 8바이트 헤더 토큰을 복사한다
            foreach (RawTileGroupView group in groups)
            {
                if (group.Entries.Count == 0)
                {
                    throw new ArgumentException("표시 묶음 저장소가 비었습니다");
                }
                RenderCollectionEntry first = group.Entries[0];
                work.Add(new(first.X, first.Y, group.Token, first.Depth, 1, 0));
            }
        }
        if (hooks.Objects is not null)
        {
            Append(hooks.Objects(request.Visible, Remaining()), Remaining());
        }
        if (hooks.Preview is not null)
        {
            Append(hooks.Preview(request.Unclipped, Remaining()), Remaining());
        }
        return new(work.Count, covered, work);
    }

    /// <summary>상위 항목과 묶음 내부의 두 순회를 구분한다. 내부 kind/tail은 sprite 그리기 경계가 해석한다.</summary>
    /// <param name="entries">현재 목록의 항목이다. 그리는 도중 새로 수집하지 않는다.</param>
    /// <param name="hideGroups">그룹 단위 출력을 생략할지.</param>
    /// <param name="debugAfterEach">디버그 후속을 각 상위 항목마다 호출할지.</param>
    /// <param name="hooks">종류별 출력 장치다.</param>
    public static void DispatchDrawEntries(IReadOnlyList<RenderCollectionEntry> entries, bool hideGroups, bool debugAfterEach, RawDrawDispatchHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(entries);
        ArgumentNullException.ThrowIfNull(hooks);
        // 원본은 현재 목록의 항목 수만 처리한다
        foreach (RenderCollectionEntry entry in entries)
        {
            switch (entry.Kind)
            {
                case 0: Required(hooks.Sprite)(entry); break;
                case 1:
                    if (!hideGroups)
                    {
                        RawTileGroupView group = Required(hooks.Group)(entry.Token);
                        // 그리기 콜백이 현재 개수를 바꿔도 원래 배열 안에서만 계속 진행한다
                        for (int index = 0; index < group.Count; index++)
                        {
                            if (index >= group.Entries.Count)
                            {
                                throw new InvalidOperationException("표시 묶음 개수가 멤버를 넘었습니다");
                            }
                            Required(hooks.Sprite)(group.Entries[index]);
                        }
                    }
                    break;
                case 2: Required(hooks.Ui)(entry.Token); break;
                case 3: Required(hooks.Construction)(entry.Token); break;
                case 4: Required(hooks.Preview)(entry); break;
                default: break;
            }
            if (debugAfterEach)
            {
                Required(hooks.AfterEach)();
            }
        }
    }
}
