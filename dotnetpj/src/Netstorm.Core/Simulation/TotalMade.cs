namespace Netstorm.Core.Simulation;

/// <summary>원본 10.78 의 debug assert 를 진단으로 보고하는 종류(실제 진단이며 보고 뒤에도 계산은 계속한다).</summary>
public enum TotalMadeAssertion
{
    /// <summary>getter 의 타입이 1 이상 numTypes 미만이 아니다.</summary>
    InvalidType,

    /// <summary>부류별 합계에 0 마스크를 넘겼다.</summary>
    ZeroGenus,
}

/// <summary>
/// 원본 Totalmade(004c23c0~004c2560)의 허용 표와 세 누적 장부: 로컬 소유자의 <b>현재 수</b>(삭제 때 감소),
/// 로컬 소유자의 <b>누적 제작 수</b>(감소 없음), 모든 소유자의 <b>전체 현재 수</b>(삭제 때 감소). 모두 타입 번호로 접근하는 256칸 DWORD 표다.
/// </summary>
/// <remarks>
/// 원본 공통 postPop(004b0d30)이 로컬 소유자 객체를 등록할 때 앞의 두 표를, 모든 소유자 객체를 등록할 때 전체 표를 올리고
/// (<see cref="RecordPopped"/>), 삭제 준비(PreDestroy)가 일반 객체에 한해 현재 수만 내린다(<see cref="RecordDestroyed"/>).
/// 누적 제작 수는 줄이지 않는다. 허용 값은 bool 이 아니라 금지 DWORD 의 0/1 로 뒤집어 저장한다. 합산은 부호 있는 넘침 대신
/// DWORD 감김을 따른다. 원본 10.78 실제 기계어 52개 관찰과 표 전체 바이트 단위로 대조한다
/// (cpppj/tests/fixtures/tutorial-one-x86.tsv, X86TotalMadeTests). CD 10.72 는 대상이 아니다.
/// 근거: docs/exe/cpp-tutorial-one-20261015.md, docs/exe/cpp-postpop-reconstruction.md. 2026-10-15 감사 D14 에서 cpppj RawTotalMade 를 옮겨 추가.
/// </remarks>
public sealed class TotalMade
{
    /// <summary>세 장부와 허용 표의 물리 칸 수(타입 번호 0~255).</summary>
    public const int TableSize = 256;

    /// <summary>원본 rep stosd 가 초기화하는 앞쪽 칸 수. 256번째 물리 칸은 초기화 범위 밖이라 유지한다.</summary>
    public const int InitializedEntries = 255;

    /// <summary>부류 합산이 시작하는 첫 실제 자산 타입 번호(내장 타입 0~69 는 건너뛴다).</summary>
    public const int FirstAssetType = 70;

    /// <summary>원본 10.78 의 전체 타입 수. 이 크기가 아닌 타입 표는 거부한다.</summary>
    public const int NativeTypeCount = 188;

    /// <summary>타입 번호 → 플래그2. 부류 합산이 현재 값을 읽는다.</summary>
    private readonly uint[] _typeFlags2;

    /// <summary>진단 보고 경계. 10.78 의 assert 에 실제로 닿았는데 비어 있으면 성공으로 숨기지 않고 예외를 낸다.</summary>
    private readonly Action<TotalMadeAssertion>? _assertion;

    /// <summary>로컬 소유자의 현재 수 표. 호출자가 입력을 바로 채우고 검사할 수 있게 실제 배열을 노출한다.</summary>
    public uint[] LocalCounts { get; } = new uint[TableSize];

    /// <summary>로컬 소유자의 누적 제작 수 표. 튜토리얼 단계 판정이 읽으며 삭제로 줄지 않는다.</summary>
    public uint[] LocalSecondaryCounts { get; } = new uint[TableSize];

    /// <summary>모든 소유자의 전체 현재 수 표.</summary>
    public uint[] GlobalCounts { get; } = new uint[TableSize];

    /// <summary>금지 DWORD 표(0이 허용). 메뉴·튜토리얼 명령이 공유하는 한 벌이다.</summary>
    public uint[] Restrictions { get; } = new uint[TableSize];

    /// <summary>
    /// 타입 표의 플래그2 와 진단 경계를 묶는다. 타입 수가 원본 10.78(188)과 다르면 거부한다.
    /// </summary>
    /// <param name="typeFlags2">타입 번호(0~187)별 플래그2.</param>
    /// <param name="assertion">원본 assert 의 진단 보고 경계. 잘못된 타입/빈 부류에 닿을 때만 필요하다.</param>
    public TotalMade(IReadOnlyList<uint> typeFlags2, Action<TotalMadeAssertion>? assertion = null)
    {
        ArgumentNullException.ThrowIfNull(typeFlags2);
        if (typeFlags2.Count != NativeTypeCount) throw new ArgumentException("Totalmade 타입 표의 크기가 원본 10.78 과 다릅니다.", nameof(typeFlags2));
        _typeFlags2 = [.. typeFlags2];
        _assertion = assertion;
    }

    /// <summary>
    /// 004c23c0: 0이 아니면 앞 255칸을 0(허용)으로, 0이면 1(금지)로 채운다. 허용 값은 금지 DWORD 로 뒤집어 저장한다.
    /// </summary>
    /// <param name="allowed">0이 아니면 모두 허용.</param>
    public void AllowAll(int allowed) => Array.Fill(Restrictions, allowed == 0 ? 1U : 0U, 0, InitializedEntries);

    /// <summary>
    /// 004c23e0: 타입 유효성 검사 없이 물리 표의 해당 칸에 금지 값을 쓴다. 호스트 저장소 밖 쓰기는 원본의 무검사 접근 대신 거부한다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    /// <param name="allowed">0이 아니면 허용.</param>
    public void Allow(int type, int allowed)
    {
        if (type < 0 || type >= Restrictions.Length) throw new ArgumentOutOfRangeException(nameof(type), "Totalmade 허용 표의 물리 범위를 벗어났습니다.");
        Restrictions[type] = allowed == 0 ? 1U : 0U;
    }

    /// <summary>
    /// 타입이 1 이상 numTypes 미만인지 확인한다. 10.78 은 벗어나면 진단을 보고하고(경계가 없으면 예외),
    /// 어느 쪽이든 getter 는 거짓/0 으로 돌아간다.
    /// </summary>
    private bool Valid(int type)
    {
        if (type > 0 && type < _typeFlags2.Length) return true;
        Report(TotalMadeAssertion.InvalidType);
        return false;
    }

    /// <summary>원본 진단을 보고한다. 경계가 비어 있으면 숨기지 않고 예외를 낸다.</summary>
    private void Report(TotalMadeAssertion kind)
    {
        if (_assertion == null) throw new InvalidOperationException("Totalmade 원본 진단 경계가 연결되지 않았습니다.");
        _assertion(kind);
    }

    /// <summary>004c2400: 유효한 타입의 현재 금지 DWORD 가 0일 때만 허용이다. 매 조회마다 현재 표에서 읽는다.</summary>
    /// <param name="type">타입 번호.</param>
    public bool Allowed(int type) => Valid(type) && Restrictions[type] == 0;

    /// <summary>004c2450: 세 장부의 앞 255칸만 0으로 지운다. 256번째 물리 칸은 유지한다.</summary>
    public void ResetCounts()
    {
        Array.Clear(LocalCounts, 0, InitializedEntries);
        Array.Clear(LocalSecondaryCounts, 0, InitializedEntries);
        Array.Clear(GlobalCounts, 0, InitializedEntries);
    }

    /// <summary>004c2480: 로컬 소유자의 현재 수(표의 DWORD 비트). 잘못된 타입이면 0.</summary>
    /// <param name="type">타입 번호.</param>
    public uint Local(int type) => Valid(type) ? LocalCounts[type] : 0U;

    /// <summary>004c24c0: 로컬 소유자의 누적 제작 수(표의 DWORD 비트). 잘못된 타입이면 0.</summary>
    /// <param name="type">타입 번호.</param>
    public uint Built(int type) => Valid(type) ? LocalSecondaryCounts[type] : 0U;

    /// <summary>004c2560: 모든 소유자의 전체 현재 수(표의 DWORD 비트). 잘못된 타입이면 0.</summary>
    /// <param name="type">타입 번호.</param>
    public uint Global(int type) => Valid(type) ? GlobalCounts[type] : 0U;

    /// <summary>
    /// 004c2500: 타입 70부터 끝까지 플래그2 가 마스크와 <b>한 비트라도 겹치는</b> 타입의 누적 제작 수를 DWORD 로 더한다(감김).
    /// 모든 비트 일치가 아니다. 마스크가 0이면 진단을 보고하고 0을 돌려준다.
    /// </summary>
    /// <param name="genus">플래그2 부류 마스크(예: 다리 4, 신전 0x200, 워크샾 0x4000).</param>
    /// <returns>부호 있는 32비트로 해석한 합.</returns>
    public int BuiltForGenus(uint genus)
    {
        if (genus == 0)
        {
            Report(TotalMadeAssertion.ZeroGenus);
            return 0;
        }
        uint total = 0;
        // 자산 타입마다 부류가 겹치면 누적 제작 수를 더한다(DWORD 감김).
        for (int type = FirstAssetType; type < _typeFlags2.Length; type++)
            if ((_typeFlags2[type] & genus) != 0) total = unchecked(total + LocalSecondaryCounts[type]);
        return unchecked((int)total);
    }

    /// <summary>
    /// 공통 postPop(004b0d30)의 통계 증가: 로컬 소유자 객체면 현재 수와 누적 제작 수를, 모든 소유자 객체면 전체 현재 수를 하나씩 올린다.
    /// 효과는 호출자가 "첫 등록(flags 1)이고 abstract/buried 가 아님"을 확인한 뒤 부른다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    /// <param name="ownerIsLocal">객체 소유자가 로컬 소유자인지.</param>
    public void RecordPopped(int type, bool ownerIsLocal)
    {
        CheckPhysical(type);
        if (ownerIsLocal)
        {
            LocalCounts[type] = unchecked(LocalCounts[type] + 1);
            LocalSecondaryCounts[type] = unchecked(LocalSecondaryCounts[type] + 1);
        }
        GlobalCounts[type] = unchecked(GlobalCounts[type] + 1);
    }

    /// <summary>
    /// 삭제 준비(PreDestroy)의 통계 감소: 일반 객체에 한해 로컬 소유자면 현재 수를, 모든 소유자는 전체 현재 수를 하나씩 내린다.
    /// 누적 제작 수는 줄이지 않는다. 등록된 적 없는 객체를 내리면 원본처럼 DWORD 가 감긴다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    /// <param name="ownerIsLocal">객체 소유자가 로컬 소유자인지.</param>
    public void RecordDestroyed(int type, bool ownerIsLocal)
    {
        CheckPhysical(type);
        if (ownerIsLocal) LocalCounts[type] = unchecked(LocalCounts[type] - 1);
        GlobalCounts[type] = unchecked(GlobalCounts[type] - 1);
    }

    /// <summary>통계 기록의 타입 번호가 물리 표 안인지 확인한다(원본의 무검사 접근 대신 명시 거부).</summary>
    private static void CheckPhysical(int type)
    {
        if (type < 0 || type >= TableSize) throw new ArgumentOutOfRangeException(nameof(type), "Totalmade 장부의 물리 범위를 벗어났습니다.");
    }
}
