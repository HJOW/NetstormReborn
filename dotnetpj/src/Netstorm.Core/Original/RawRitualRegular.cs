// Rune·다이스·제단 정기의 분기 본문을 같은 실제 부모/현재 장치에서 공유한다. 제단(Altar) 분기는 별도 fixture에서 잇는다.
namespace Netstorm.Core.Original;

/// <summary>RawRitualRegular.h의 304에서 선택한 현재 검색/진행/가상 삭제만 요구한다.</summary>
public sealed class RuneRegularHooks
{
    /// <summary>같은 현재 역할과 owner로 반경1을 검색한다.</summary>
    public Func<int, int>? FindDais { get; set; }
    /// <summary>발견한 실제 다이스의 전체 진행 본문이다.</summary>
    public Action<int>? Progress { get; set; }
    /// <summary>Rune의 현재 가상10에 flags48을 전달한다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다.</summary>
    internal RuneRegularHooks Copy() => (RuneRegularHooks)MemberwiseClone();
}

/// <summary>RawRitualRegular.h의 305/306/307과 공통 Damageable을 원래 선택 위치에서 호출한다.</summary>
public sealed class DaisHandleHooks
{
    /// <summary>저장한 프레임에 현재 가상 Count/단계 표를 적용한다.</summary>
    public Func<int, int, uint>? Stage { get; set; }
    /// <summary>같은 실제 Frame.Set이다.</summary>
    public Action<int, int, uint>? Frame { get; set; }
    /// <summary>실제44a200 전체 후단이다.</summary>
    public Func<int, float>? SacrificeAdvance { get; set; }
    /// <summary>현재 강한 사제 검색 뒤 실제447e40을 호출한다.</summary>
    public Action<int>? Ritual { get; set; }
    /// <summary>나머지 사건의 같은44b900 본문이다.</summary>
    public RegularHandler? Damageable { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다.</summary>
    internal DaisHandleHooks Copy() => (DaisHandleHooks)MemberwiseClone();
}

/// <summary>원본 제단의 현재 저항 지연/확률이며 Session에서 재설정하지 않는다. 0.939에서 추가됐다.</summary>
public sealed class AltarSacrificeState
{
    /// <summary>00532698의 다음 주기 덧셈 값이다.</summary>
    public float StruggleSeconds { get; set; } = 3.0F;
    /// <summary>0053269c의 원문 난수/1000 비교 값이다.</summary>
    public int EscapePercent { get; set; } = 40;
}

/// <summary>조회·프레임·난수·예약과 선택된 큰 후단을 구별한다. 0.939에서 추가됐다.</summary>
public sealed class AltarRegularHooks
{
    /// <summary>기준점의 typed 다이스를 검색한다.</summary>
    public Func<int, int>? Dais { get; set; }
    /// <summary>그 안의 현재 사제 내용물을 검색한다.</summary>
    public Func<int, int>? Priest { get; set; }
    /// <summary>각 원문 관문에서 현재 권한을 읽는다.</summary>
    public Func<bool>? Boss { get; set; }
    /// <summary>저장된 두 owner로 현재 방향별 동맹 DWORD를 읽는다.</summary>
    public Func<uint, uint, int>? Alliance { get; set; }
    /// <summary>실제 해방 전체 본문이다.</summary>
    public Action<int>? Release { get; set; }
    /// <summary>실제 애니메이션 제거 전체 본문이다.</summary>
    public Action<int>? RemoveAnimation { get; set; }
    /// <summary>다이스 이탈 프레임이다.</summary>
    public Action<int, uint>? EscapeFrame { get; set; }
    /// <summary>실제 가상 삭제이다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>실제 FrameAdvance의 구간 끝 여부를 반환한다.</summary>
    public Func<int, int, uint, bool>? Advance { get; set; }
    /// <summary>같은 Frame.Set이다.</summary>
    public Action<int, int, uint>? Frame { get; set; }
    /// <summary>원문 new28의 실제 확보 성공이다.</summary>
    public Func<bool>? ReserveShared { get; set; }
    /// <summary>같은 확보 주소에 실제 SharedRegular를 구성/부착한다.</summary>
    public Action<int, uint, float>? AddShared { get; set; }
    /// <summary>단일 현재 게임 난수를 선택된 범위에서 소비한다.</summary>
    public Func<int, int, int>? RandomInt { get; set; }
    /// <summary>단일 현재 게임 난수의 원문 실수 구간이다.</summary>
    public Func<float, float, float>? RandomFloat { get; set; }
    /// <summary>원문 위치/이름/flags 그대로 실제 소리 장치에 전달한다.</summary>
    public Action<float, float, string, uint>? Sound { get; set; }
    /// <summary>나머지 사건의 기존44b900 본문이다.</summary>
    public RegularHandler? Damageable { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다.</summary>
    internal AltarRegularHooks Copy() => (AltarRegularHooks)MemberwiseClone();
}

/// <summary>RawRitualRegular.cpp의 304/305/306/307 분기다. 제단 분기는 후속 fixture에서 잇는다.</summary>
public static class RawRitualRegular
{
    /// <summary>도달한 장치만 요구하며 호출 중 자신의 연결 교체에도 수명을 유지한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("의식 정기의 선택된 실제 효과 미연결");
    /// <summary>304에서만 검색/진행/삭제를 실행하고 나머지는 원문base 그대로 반환한다. 원문44a3a0이다.</summary>
    public static float RunRuneRegular(RawSidPool pool, int sid, uint @event, float payload, RuneRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks);
        if (@event != 0x304) return payload;
        int dais = Need(hooks.FindDais)(sid);
        if (dais != 0 && dais < pool.Capacity) Need(hooks.Progress)(dais);
        Need(hooks.Destroy)(sid, 0x48); return -1.0F;
    }
    /// <summary>저장한 frame과 next를 현재 단계 조회 두 번에 원문 순서로 전달한다. 원문44b150이다.</summary>
    public static float RunDaisHandle(RawSidPool pool, int sid, uint @event, uint count, float payload, DaisHandleHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks);
        if (@event == 0x305)
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            int frame = pool.Edition == OriginalEdition.Patch1078 ? System.Buffers.Binary.BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34];
            int next = unchecked(frame + 1);
            uint nextStage = Need(hooks.Stage)(sid, next), stage = Need(hooks.Stage)(sid, frame);
            Need(hooks.Frame)(sid, next, 0); return stage == nextStage ? 0.1F : 0;
        }
        if (@event == 0x306) return Need(hooks.SacrificeAdvance)(sid);
        if (@event == 0x307 && count != 0) { Need(hooks.Ritual)(sid); return 0; }
        return Need(hooks.Damageable)(sid, @event, count, payload);
    }

    /// <summary>44a5e0 전체 분기를 기존 typed/Native에서 함께 사용한다. 0.939에서 추가됐다.</summary>
    /// <param name="pool">같은 실제 SID 풀이다.</param>
    /// <param name="frames">같은 실제 TYPE 프레임 코드표다.</param>
    /// <param name="altar">같은 실제 제단 번호다.</param>
    /// <param name="event">원문 입력 사건 번호다.</param>
    /// <param name="count">원문 입력 개수다.</param>
    /// <param name="payload">원문 입력 전달값이다.</param>
    /// <param name="state">원본 제단의 현재 저항 지연/확률이다.</param>
    /// <param name="hooks">실제 조회/프레임/난수/예약 장치다.</param>
    public static float RunAltarRegular(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, int altar, uint @event, uint count, float payload, AltarSacrificeState state, AltarRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        const float finishedDelay = 3.0F;
        const float strugglePayload = 0.08F;
        const uint sacrificeDestroy = 0x400000u;
        const uint playerCount = 8u;
        string[] chains = ["chainRattle23.wav", "chainRattle24.wav", "chainRattle25.wav", "chainRattle26.wav", "chainRattle27.wav"];
        string[] struggles = ["priestStruggleFade01.wav", "priestStruggleFade02.wav", "priestStruggleFade03.wav"];
        // 원본 유효 주소 범위만 판정한다.
        static bool Valid(RawSidPool pool, int sid) => sid > 0 && sid < pool.Capacity;
        // 선택 시 현재 두 좌표를 읽는다.
        static (float X, float Y) Position(RawSidPool pool, int sid)
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            return (System.Buffers.Binary.BinaryPrimitives.ReadSingleLittleEndian(raw.Slice(14, 4)), System.Buffers.Binary.BinaryPrimitives.ReadSingleLittleEndian(raw.Slice(18, 4)));
        }
        // 실제 현재 TYPE/프레임 코드다.
        static int Direction(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, int sid)
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            int type = raw[10];
            if (type < 0 || type >= frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "제단 현재 방향 TYPE 물리 범위");
            int frame = pool.Edition == OriginalEdition.Patch1078 ? System.Buffers.Binary.BinaryPrimitives.ReadInt32LittleEndian(raw.Slice(36, 4)) : raw[34];
            return RawPathAnimation.Direction(frames[type], frame);
        }
        // 저장한 대상의 현재 글자 첫번호다.
        static int FirstFrame(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, int sid, byte letter)
        {
            int type = pool.Slot(sid)[10];
            if (type < 0 || type >= frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "제단 현재 frame TYPE 물리 범위");
            return frames[type].Run(letter).First;
        }
        int dais = Need(hooks.Dais)(altar);
        bool hasDais = Valid(pool, dais);
        float centerX = 0, centerY = 0;
        if (hasDais)
        {
            (float pointX, float pointY) = Position(pool, dais);
            centerX = pointX - finishedDelay;
            centerY = pointY - finishedDelay;
        }

        if (@event == 21)
        {
            if (pool.Edition == OriginalEdition.Patch1078)
            {
                int captured = Need(hooks.Priest)(dais);
                if (Need(hooks.Boss)() && Valid(pool, captured))
                {
                    uint oldOwner = (uint)(pool.Slot(captured)[12] & 127), owner = pool.Slot(dais)[34];
                    if (oldOwner > playerCount || owner > playerCount) throw new ArgumentOutOfRangeException(nameof(altar), "제단 자동 해방 동맹 인덱스");
                    if (Need(hooks.Alliance)(oldOwner, owner) != 0) { Need(hooks.Release)(altar); return -1.0F; }
                }
            }

            if (Direction(pool, frames, altar) == 15) return payload;
            uint nextEvent = 20;
            if (Need(hooks.RandomInt)(0, 100000) / 1000 < state.EscapePercent) { nextEvent = 22; Need(hooks.Advance)(altar, -1, 0); }
            if (Need(hooks.ReserveShared)()) Need(hooks.AddShared)(altar, nextEvent, strugglePayload);
            if (hasDais && Direction(pool, frames, dais) != 10)
            {
                int chain = Need(hooks.RandomInt)(0, 5);
                if (chain < 0 || chain >= chains.Length) throw new ArgumentOutOfRangeException(nameof(hooks), "제단 chain 난수 범위");
                Need(hooks.Sound)(centerX, centerY, chains[chain], 0);
                int struggle = Need(hooks.RandomInt)(0, 3000) / 1000;
                if (struggle < 0 || struggle >= struggles.Length) throw new ArgumentOutOfRangeException(nameof(hooks), "제단 struggle 난수 범위");
                Need(hooks.Sound)(centerX, centerY, struggles[struggle], 0);
            }

            return Need(hooks.RandomFloat)(0, 2) + state.StruggleSeconds;
        }

        if (@event == 20 || @event == 22)
        {
            if (Direction(pool, frames, altar) != 14) return 0;
            int delta = @event == 20 ? 1 : -1;
            if (Need(hooks.Advance)(altar, delta, 0)) { if (delta == -1) Need(hooks.Advance)(altar, 1, 0); return 0; }
            return payload;
        }

        if (@event == 23) return payload;
        if (@event != 24) return Need(hooks.Damageable)(altar, @event, count, payload);
        dais = Need(hooks.Dais)(altar);
        if (!Valid(pool, dais)) throw new InvalidOperationException("제단24 원본의 다이스0 프레임읽기 경계");
        if (Direction(pool, frames, dais) == 10)
        {
            if (Need(hooks.Boss)()) { Need(hooks.Destroy)(dais, sacrificeDestroy); return -1.0F; }
            return payload;
        }

        if (!Need(hooks.Advance)(altar, 1, 0))
        {
            if (count < 3 || Need(hooks.RandomInt)(0, 100000) / 1000 < 50) { if (Valid(pool, dais)) Need(hooks.EscapeFrame)(dais, count); }
            if (Need(hooks.RandomFloat)(0, 1) < payload) { (float pointX, float pointY) = Position(pool, altar); Need(hooks.Sound)(pointX, pointY, "altarBurnCollapse.wav", 0); }
            return payload;
        }

        dais = Need(hooks.Dais)(altar);
        if (!Valid(pool, dais)) return payload;
        if (Need(hooks.Boss)())
        {
            int captured = Need(hooks.Priest)(dais);
            if (captured != 0) Need(hooks.Destroy)(captured, 8);
        }

        Need(hooks.RemoveAnimation)(altar);
        (float altarX, float altarY) = Position(pool, altar);
        Need(hooks.Sound)(altarX, altarY, "explosion.wav", 0);
        Need(hooks.Frame)(dais, FirstFrame(pool, frames, dais, (byte)'K'), 0);
        Need(hooks.Frame)(altar, FirstFrame(pool, frames, altar, (byte)'M'), 0);
        return finishedDelay;
    }
}
