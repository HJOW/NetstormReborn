using System.Buffers.Binary;

// 현재 Carrier 내용물의 폭탄/사제/지면 검사/해방 통지 순서를 복원한다.
namespace Netstorm.Core.Original;

/// <summary>원문 읽기 위치마다 다시 조회하는 Carrier 내용물 전역이다.</summary>
public sealed class CarrierContentNoticeState
{
    /// <summary>현재 저장 게임 관문이다.참이면 나머지 인자를 읽지 않는다.</summary>
    public Func<bool>? SavedGame { get; set; }
    /// <summary>현재 로딩 깊이다.비영이면 모든 후속 효과를 생략한다.</summary>
    public Func<uint>? LoadingDepth { get; set; }
    /// <summary>폭탄의 번개/음향/숫자 효과만 억제하는 현재 DWORD다.</summary>
    public Func<uint>? BombBlocked { get; set; }
    /// <summary>현재 부모 owner BYTE와 비교하는 로컬 플레이어다.</summary>
    public Func<uint>? LocalOwner { get; set; }
    /// <summary>현재 포획/해방 사제 역할 DWORD다.</summary>
    public Func<uint>? PriestType { get; set; }
    /// <summary>투영 시 읽는 현재 픽셀 카메라 X다.</summary>
    public Func<int>? CameraX { get; set; }
    /// <summary>투영 시 읽는 현재 픽셀 카메라 Y다.</summary>
    public Func<int>? CameraY { get; set; }
    /// <summary>해방 소리를 억제하는 현재 Rain Walker 역할이다.</summary>
    public Func<uint>? FreeSoundBlockerType { get; set; }
}

/// <summary>원문 계산을 대신하지 않는 실제 화면/음향/프로세스/공간 효과다.</summary>
public sealed class CarrierContentNoticeHooks
{
    /// <summary>현재 부모의 실제 중심이다.원문의 두 호출을 합치지 않는다.</summary>
    public Func<int, (float X, float Y)>? Center { get; set; }
    /// <summary>현재 부모·투영 시작/끝 좌표·5초 번개 효과다.</summary>
    public Action<int, (int X, int Y), (int X, int Y), float>? Bolt { get; set; }
    /// <summary>현재 중심에서 thunderCrack.wav를 재생한다.</summary>
    public Action<float, float, string>? SoundAt { get; set; }
    /// <summary>현재 TYPE+6c/+7c의 실제 추가 DWORD를 읽는다.</summary>
    public Func<uint, uint, uint>? TypeWord { get; set; }
    /// <summary>원문40바이트 예약의 실제 토큰이며0은 실패다.</summary>
    public Func<uint, uint>? Allocate { get; set; }
    /// <summary>예약·부모·소리·대체0·flags12의 실제 Sound47 생성이다.</summary>
    public Func<uint, int, uint, uint, uint, uint>? Create { get; set; }
    /// <summary>생성된 실제 프로세스 타이머에1.5초를 설정한다.</summary>
    public Action<uint, float>? Duration { get; set; }
    /// <summary>현재 부모 TYPE+7c의 소리를 재생한다.</summary>
    public Action<uint>? Sound { get; set; }
    /// <summary>같은 실제 Carrier의 지면/내용물을 검사한다.</summary>
    public Action<int>? CheckCarrier { get; set; }
    /// <summary>현재 좌표/부류의 첫 실제 객체를 찾는다.</summary>
    public Func<float, float, uint, int>? GenusAt { get; set; }
    /// <summary>현재 좌표/TYPE의 첫 실제 객체를 찾는다.</summary>
    public Func<float, float, uint, int>? TypeAt { get; set; }
    /// <summary>priestFree.wav의 원문 전역6인자 재생을 연결한다.</summary>
    public Action<string>? FreeSound { get; set; }
    /// <summary>실행 중 외부 재연결과 현재 callable을 분리하는 값 복사다.</summary>
    public CarrierContentNoticeHooks Copy() => (CarrierContentNoticeHooks)MemberwiseClone();
}

/// <summary>cpppj RawCarrierContentNotice의 PATCH00429560 전체 분기 본체다.</summary>
public sealed class RawCarrierContentNotice
{
    /// <summary>같은 실제50바이트 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>효과 뒤 현재 kind로 다시 읽는 실제188개 TYPE다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원문 위치에서 현재 전역을 읽는 뷰다.</summary>
    private readonly CarrierContentNoticeState _state;
    /// <summary>선택된 실제 외부 효과의 함수 사본이다.</summary>
    private readonly CarrierContentNoticeHooks _hooks;
    /// <summary>동일 부모의 실제 가상 진입점을 조회한다.</summary>
    private readonly RawContentParentNotice _dispatch;
    /// <summary>같은 PATCH 풀/TYPE와 장치를 빌리고 생성 시 게임 상태는 읽지 않는다.</summary>
    public RawCarrierContentNotice(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, CarrierContentNoticeState state, CarrierContentNoticeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188) throw new ArgumentException("Carrier 통지의 PATCH 실제 TYPE/풀 연결 오류");
        _pool = pool; _types = types; _state = state; _hooks = hooks.Copy(); _dispatch = new(pool);
    }
    /// <summary>현재 실제 부모 가상+24가429560인지만 조회한다.</summary>
    public bool Handles(int parent) => _dispatch.NativeEntry(parent) == 0x429560;
    /// <summary>선택된 실제 하위가 없으면 원문 호출 위치에서 실패한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("Carrier 내용물의 선택된 실제 효과/전역 미연결");
    /// <summary>필요한 BYTE/WORD/DWORD만 같은 현재 슬롯에서 읽는다.</summary>
    private uint Read(int sid, int offset, int width = 2)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid)[offset..];
        return width switch { 1 => raw[0], 2 => BinaryPrimitives.ReadUInt16LittleEndian(raw), 4 => BinaryPrimitives.ReadUInt32LittleEndian(raw), _ => throw new ArgumentOutOfRangeException(nameof(width)) };
    }
    /// <summary>현재 kind WORD의 현재 TYPE를 물리 범위 안에서 조회한다.</summary>
    private OriginalTypeRecord ContentType(int form)
    {
        uint kind = Read(form, 18); return kind < _types.Count ? _types[(int)kind] : throw new ArgumentOutOfRangeException(nameof(form), "Carrier 내용물의 현재 TYPE 물리 범위 오류");
    }
    /// <summary>원문16/11배·0.5·CRT 하위DWORD와 카메라 뺄셈 감김을 유지한다.</summary>
    private static int ProjectCoordinate(float coordinate, int scale, int camera)
    {
        double value = (double)coordinate * scale + .5; long integer = 0;
        if (double.IsFinite(value) && value >= -9223372036854775808d && value < 9223372036854775808d) integer = (long)value;
        return unchecked((int)(uint)integer - camera);
    }
    /// <summary>같은 중심을 투영하되 현재 원점은 두 투영 각각에서 다시 읽는다.</summary>
    private (int X, int Y) Project((float X, float Y) point) => (ProjectCoordinate(point.X, 16, Need(_state.CameraX)()), ProjectCoordinate(point.Y, 11, Need(_state.CameraY)()));
    /// <summary>해당 원문 위치에서 현재 부모의 두 float 비트를 읽는다.</summary>
    private (float X, float Y) Position(int parent) => (BitConverter.UInt32BitsToSingle(Read(parent, 14, 4)), BitConverter.UInt32BitsToSingle(Read(parent, 18, 4)));
    /// <summary>saved/load 접두 뒤 폭탄·포획음·지면 검사·해방음을 원문 단락평가/재조회 순서로 실행한다.</summary>
    public void Notice(int parent, int form, uint reason)
    {
        if (Need(_state.SavedGame)()) return;
        if (Need(_state.LoadingDepth)() != 0) return;
        if ((Read(form, 11, 1) & 2) == 0 && (ContentType(form).Flags2 & 0x100) != 0 && Need(_state.BombBlocked)() == 0)
        {
            var center = Need(_hooks.Center)(parent); var first = Project(center); var second = Project(center);
            Need(_hooks.Bolt)(parent, (first.X, 0), second, 5);
            var soundCenter = Need(_hooks.Center)(parent); Need(_hooks.SoundAt)(soundCenter.X, soundCenter.Y, "thunderCrack.wav");
            uint value = Need(_hooks.TypeWord)(Read(form, 18), 0x6c);
            if (Read(parent, 34, 1) == Need(_state.LocalOwner)())
            {
                uint storage = Need(_hooks.Allocate)(40), process = 0;
                if (storage != 0) process = Need(_hooks.Create)(storage, parent, value, 0, 12);
                if (process != 0) Need(_hooks.Duration)(process, 1.5f);
            }
        }
        else if (Read(parent, 34, 1) == Need(_state.LocalOwner)() && Read(form, 18) == Need(_state.PriestType)() && (Read(form, 11, 1) & 2) == 0)
        {
            uint sound = Need(_hooks.TypeWord)(Read(parent, 10, 1), 0x7c); Need(_hooks.Sound)(sound);
        }
        if ((Read(form, 11, 1) & 2) == 0 && Need(_state.LoadingDepth)() == 0 && (reason & 1) != 0 && Read(form, 20) != 0 && (ContentType(form).Flags2 & 0x208100) != 0)
            Need(_hooks.CheckCarrier)(parent);
        if ((reason & 2) != 0 && Read(form, 18) == Need(_state.PriestType)())
        {
            var position = Position(parent);
            if (Need(_hooks.GenusAt)(position.X, position.Y, 0x400000) == 0)
            {
                var current = Position(parent);
                if (Need(_hooks.TypeAt)(current.X, current.Y, Need(_state.FreeSoundBlockerType)()) == 0) Need(_hooks.FreeSound)("priestFree.wav");
            }
        }
    }
}
