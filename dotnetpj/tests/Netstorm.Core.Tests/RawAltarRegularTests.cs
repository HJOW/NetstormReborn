// 제단 정기의 원문 분기와 큰 후단 선택을 같은 풀/프레임에서 대조한다.
// cpppj/src/o/RawRitualRegular.cpp 의 RunAltarRegular(44a5e0) 를 대조한다.
// 0.939에서 R4-4 정기 처리 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// 0044a5e0 제단 분기의 실제 장치 호출과 반환 payload를 검사한다.
public sealed class RawAltarRegularTests
{
    /// <summary>같은 실제 풀/프레임/장치를 빌린다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 SID 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false, edition: OriginalEdition.Patch1078);
        /// <summary>같은 실제 TYPE 프레임 코드표다.</summary>
        public readonly RiftTypeFrames[] Frames;
        /// <summary>실제 장치 호출 기록이다.</summary>
        public readonly List<string> Calls = [];
        /// <summary>실제 Dais 조회 결과다.</summary>
        public int DaisResult = 6;
        /// <summary>실제 사제 조회 결과다.</summary>
        public int PriestResult = 7;
        /// <summary>실제 권한 결과다.</summary>
        public bool BossResult;
        /// <summary>실제 동맹 결과다.</summary>
        public int AllianceResult;
        /// <summary>실제 구간 끝 결과다.</summary>
        public bool AdvanceResult = true;
        /// <summary>실제 확보 결과다.</summary>
        public bool ReserveResult = true;
        /// <summary>실제 정수 난수 결과다.</summary>
        public int RandomIntResult;
        /// <summary>실제 실수 난수 결과다.</summary>
        public float RandomFloatResult = 1.5f;

        /// <summary>문자 패턴으로 프레임 코드표를 만든다.</summary>
        private static RiftTypeFrames Pattern(string pattern)
        {
            var codes = new List<RiftFrameCode>();
            for (int index = 0; index < pattern.Length; index++) codes.Add(new RiftFrameCode((byte)pattern[index], (byte)'P', (byte)(index + 1), 0));
            return new RiftTypeFrames(codes);
        }

        /// <summary>제단/다이스 글자를 고정한 코드표를 둔다.</summary>
        public Scene()
        {
            Frames = [new RiftTypeFrames([]), Pattern("A"), Pattern("A"), Pattern("A")];
            foreach (int sid in new[] { 5, 6, 7 }) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }

        /// <summary>같은 실제 슬롯 바이트를 쓴다.</summary>
        public void Insert(int sid, uint type, int frame = 0, float x = 40.75f, float y = 41.25f, uint owner34 = 1, uint owner12 = 1)
        {
            Span<byte> raw = Pool.RawSlot(sid);
            raw.Clear();
            raw[10] = (byte)type;
            BinaryPrimitives.WriteInt32LittleEndian(raw[36..], frame);
            BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x);
            BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
            raw[34] = (byte)owner34;
            raw[12] = (byte)owner12;
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public AltarRegularHooks Hooks() => new()
        {
            Dais = _ => { Calls.Add("dais"); return DaisResult; },
            Priest = _ => { Calls.Add("priest"); return PriestResult; },
            Boss = () => { Calls.Add("boss"); return BossResult; },
            Alliance = (_, _) => { Calls.Add("alliance"); return AllianceResult; },
            Release = altar => Calls.Add("release:" + altar),
            RemoveAnimation = altar => Calls.Add("remove:" + altar),
            EscapeFrame = (dais, count) => Calls.Add($"escape:{dais}:{count}"),
            Destroy = (sid, flags) => Calls.Add($"destroy:{sid}:{flags}"),
            Advance = (sid, delta, _) => { Calls.Add($"advance:{sid}:{delta}"); return AdvanceResult; },
            Frame = (sid, frame, flags) => Calls.Add($"frame:{sid}:{frame}:{flags}"),
            ReserveShared = () => { Calls.Add("reserve"); return ReserveResult; },
            AddShared = (sid, ev, payload) => Calls.Add($"shared:{sid}:{ev}:{payload}"),
            RandomInt = (_, _) => { Calls.Add("randomInt"); return RandomIntResult; },
            RandomFloat = (_, _) => { Calls.Add("randomFloat"); return RandomFloatResult; },
            Sound = (x, y, name, flags) => Calls.Add($"sound:{x}:{y}:{name}:{flags}"),
            Damageable = (sid, ev, count, payload) => { Calls.Add($"damageable:{sid}:{ev}:{count}:{payload}"); return 7.5f; },
        };
    }

    /// <summary>21번은 도망 예약과 다섯·세 소리를 내고 주기를 더해 반환한다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void Event21EscapeReservesSharedAndPlaysSounds()
    {
        var scene = new Scene();
        scene.Insert(5, 1);
        scene.Insert(6, 2, owner34: 2);
        scene.Insert(7, 3, owner12: 3);
        var state = new AltarSacrificeState();
        float result = RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 21, 0, 0.25f, state, scene.Hooks());
        Assert.Equal(4.5f, result);
        Assert.Contains("advance:5:-1", scene.Calls);
        Assert.Contains("shared:5:22:0.08", scene.Calls);
        Assert.Contains("sound:37.75:38.25:chainRattle23.wav:0", scene.Calls);
        Assert.Contains("sound:37.75:38.25:priestStruggleFade01.wav:0", scene.Calls);
        Assert.DoesNotContain(scene.Calls, call => call.StartsWith("release:", StringComparison.Ordinal));
    }

    /// <summary>21번은 동맹 주인이면 제단을 해방하고 -1을 반환한다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void Event21AutoReleaseOnAlliedOwner()
    {
        var scene = new Scene();
        scene.Insert(5, 1);
        scene.Insert(6, 2, owner34: 2);
        scene.Insert(7, 3, owner12: 3);
        scene.BossResult = true;
        scene.AllianceResult = 1;
        float result = RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 21, 0, 0.25f, new AltarSacrificeState(), scene.Hooks());
        Assert.Equal(-1.0f, result);
        Assert.Contains("release:5", scene.Calls);
    }

    /// <summary>21번은 방향 15에서 원문 payload를 그대로 돌려준다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void Event21Direction15ReturnsPayload()
    {
        var scene = new Scene();
        scene.Frames[1] = new RiftTypeFrames([new RiftFrameCode((byte)'P', (byte)'P', 1, 0)]);
        scene.Insert(5, 1);
        scene.Insert(6, 2, owner34: 2);
        scene.Insert(7, 3, owner12: 3);
        float result = RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 21, 0, 0.5f, new AltarSacrificeState(), scene.Hooks());
        Assert.Equal(0.5f, result);
        Assert.DoesNotContain(scene.Calls, call => call.StartsWith("sound:", StringComparison.Ordinal));
    }

    /// <summary>20번은 구간이 끝나면 0을 반환한다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void Event20AdvanceReturnsZero()
    {
        var scene = new Scene();
        scene.Frames[1] = new RiftTypeFrames([new RiftFrameCode((byte)'O', (byte)'P', 1, 0)]);
        scene.Insert(5, 1);
        float result = RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 20, 0, 0.5f, new AltarSacrificeState(), scene.Hooks());
        Assert.Equal(0.0f, result);
        Assert.Contains("advance:5:1", scene.Calls);
    }

    /// <summary>22번은 구간이 끝나면 뒤로 물리고 0을 반환한다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void Event22RetreatsAndRestores()
    {
        var scene = new Scene();
        scene.Frames[1] = new RiftTypeFrames([new RiftFrameCode((byte)'O', (byte)'P', 1, 0)]);
        scene.Insert(5, 1);
        float result = RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 22, 0, 0.5f, new AltarSacrificeState(), scene.Hooks());
        Assert.Equal(0.0f, result);
        Assert.Contains("advance:5:-1", scene.Calls);
        Assert.Contains("advance:5:1", scene.Calls);
    }

    /// <summary>23번은 원문 payload를 그대로 돌려준다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void Event23ReturnsPayload()
    {
        var scene = new Scene();
        scene.Insert(5, 1);
        float result = RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 23, 0, 0.5f, new AltarSacrificeState(), scene.Hooks());
        Assert.Equal(0.5f, result);
        Assert.Equal(["dais"], scene.Calls);
    }

    /// <summary>24번은 다이스가 시작 자세면 파괴하고 -1을 반환한다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void Event24FinishDestroysDais()
    {
        var scene = new Scene();
        scene.Frames[2] = new RiftTypeFrames([new RiftFrameCode((byte)'K', (byte)'P', 1, 0)]);
        scene.Insert(5, 1);
        scene.Insert(6, 2);
        scene.BossResult = true;
        float result = RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 24, 0, 0.5f, new AltarSacrificeState(), scene.Hooks());
        Assert.Equal(-1.0f, result);
        Assert.Contains("destroy:6:4194304", scene.Calls);
    }

    /// <summary>24번은 전진이 막히면 확률·소리 없이 payload를 돌려준다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void Event24ContinuesOnFailedAdvance()
    {
        var scene = new Scene();
        scene.Insert(5, 1);
        scene.Insert(6, 2);
        scene.AdvanceResult = false;
        scene.RandomIntResult = 60000;
        scene.RandomFloatResult = 0.9f;
        float result = RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 24, 5, 0.5f, new AltarSacrificeState(), scene.Hooks());
        Assert.Equal(0.5f, result);
        Assert.DoesNotContain(scene.Calls, call => call.StartsWith("escape:", StringComparison.Ordinal));
        Assert.DoesNotContain(scene.Calls, call => call.StartsWith("destroy:", StringComparison.Ordinal));
    }

    /// <summary>그 밖의 사건은 같은 손상 본문에 전달한다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void OtherEventFallsThroughToDamageable()
    {
        var scene = new Scene();
        scene.Insert(5, 1);
        float result = RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 99, 2, 0.5f, new AltarSacrificeState(), scene.Hooks());
        Assert.Equal(7.5f, result);
        Assert.Contains("damageable:5:99:2:0.5", scene.Calls);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.939에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        scene.Insert(5, 1);
        Assert.Throws<ArgumentNullException>(() => RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 21, 0, 0, new AltarSacrificeState(), null!));
        var bare = new AltarRegularHooks();
        Assert.Throws<InvalidOperationException>(() => RawRitualRegular.RunAltarRegular(scene.Pool, scene.Frames, 5, 21, 0, 0, new AltarSacrificeState(), bare));
        Assert.Throws<ArgumentNullException>(() => RawRitualRegular.RunAltarRegular(null!, scene.Frames, 5, 21, 0, 0, new AltarSacrificeState(), scene.Hooks()));
    }
}
