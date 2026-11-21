// 실제 Kernel 프레임을 같은 Runtime/현재 공간/파생 프레임으로 분배한다.
namespace Netstorm.Core.Original;

/// <summary>RawPathProcess.cpp의 프레임/최종 도착 본체다.</summary>
public sealed partial class RawPathProcess
{
    /// <summary>같은 실제 Step과 현재 부모 위치/소리 번호를 사용한다. 벌룬은 원문처럼 CD 관찰 인자를 생략한다.</summary>
    private RawPathSample Step(int parent, bool observationStamp = true) => Runtime.Step(_state.Clock.Now, RawPathSlot.Position(_pool, parent), _state.Observation,
        () => { RawPathPoint point = RawPathSlot.Position(_pool, parent); _hooks.StepSound!(parent, point.X, point.Y, _hooks.SoundId!(parent)); }, observationStamp ? _hooks.ObservationStamp?.Invoke() ?? 0 : 0);
    /// <summary>실제 immobile/시각/가상종류 분기 뒤 현재 raw 공간과 프레임을 원문 순서로 갱신한다.</summary>
    public override void RunFrame()
    {
        _ = CurrentHost(); int parent = Parent(); PathLifeWatch watch = _watch;
        if (_walker)
        {
            RawWalkerFrame.Run(_pool, parent, Runtime, _state.Clock.Now, _frames, _state.WindWalkerType, _state.RainWalkerType, ref _state.WindWalkerCounter, new()
            {
                Immobile = () => (_types[_pool.Slot(parent)[10]].Flags2 & 0x10000) != 0 && CurrentHost().FindEvent(parent, 0x5105) is not null,
                Halt = Halt, Step = () => Step(parent), GroundStop = GroundStop, Arrive = Arrive, Unpop = _hooks.Unpop, Pop = _hooks.Pop,
            }, watch); return;
        }
        if (_bulf)
        {
            RawBulfFrame.Run(_pool, parent, Runtime, _frames, _shapes, new()
            {
                Immobile = () => (_types[_pool.Slot(parent)[10]].Flags2 & 0x10000) != 0 && CurrentHost().FindEvent(parent, 0x5105) is not null,
                Halt = Halt, Step = () => Step(parent), Arrive = Arrive, Unpop = _hooks.Unpop, Pop = _hooks.Pop, RandomInt = _hooks.BulfRandom, Sound = _hooks.BulfSound,
            }, watch); return;
        }
        if (_balloon)
        {
            RawBalloonFrame.Run(_pool, parent, Runtime, () => _state.Clock.Now, _state.BalloonType, new()
            {
                Halt = Halt, Step = () => Step(parent, false), Arrive = Arrive, Unpop = _hooks.Unpop, Pop = _hooks.Pop,
                Flame = (x, y) => Need(_hooks.BalloonFlame)(x, y),
            }, watch); return;
        }
        byte type = _pool.Slot(parent)[10];
        if ((_priest || _worm || _dude) && (_types[type].Flags2 & 0x10000) != 0 && CurrentHost().FindEvent(parent, 0x5105) is not null) return;
        if (Runtime.Finished || Halt()) return;
        if (_flying) { if (Runtime.NextFrame > _state.Clock.Now) return; Runtime.NextFrame = _state.Clock.Now + 0.019999999552965164; }
        RawPathSample sample = Step(parent); if (!watch.Alive) return; if (sample.Finished) { Arrive(); return; }
        if (!(_priest || _worm || _dude))
        {
            RawPathPoint position = RawPathSlot.Position(_pool, parent); if (sample.Position.X != position.X || sample.Position.Y != position.Y) { _hooks.Unpop!(parent, 0); _hooks.Pop!(parent, sample.Position, 0x40); } return;
        }
        if (GroundStop(sample.Position) || !watch.Alive || (_pool.Slot(parent)[11] & 1) != 0) return;
        RawPathPoint current = RawPathSlot.Position(_pool, parent); int originalFrame = RawPathSlot.Frame(_pool, parent), nextFrame = originalFrame;
        if (originalFrame < 0 || originalFrame >= _shapes[type].Frames.Count) throw new ArgumentOutOfRangeException(nameof(parent), "경로 SHP 프레임");
        SquidDisplayFrame shape = _shapes[type].Frames[originalFrame]; float footX = (float)((double)current.X + shape.WalkStepX), footY = (float)((double)current.Y + shape.WalkStepY); int direction = Runtime.Direction(Runtime.Current);
        if ((double)sample.Angle % 45.0 == 0)
        {
            double dx = (double)sample.Position.X - current.X, dy = (double)sample.Position.Y - current.Y, hx = (double)footX - current.X, hy = (double)footY - current.Y;
            if (hx * hx + hy * hy < dx * dx + dy * dy)
            {
                if (direction < 0 || direction > 15) throw new ArgumentOutOfRangeException(nameof(parent), "경로 프레임 글자");
                RiftTypeFrames.LetterRun run = _frames[type].Run((byte)('A' + direction)); if (run.Count == 0) throw new ArgumentOutOfRangeException(nameof(parent), "경로 프레임 구간0");
                nextFrame = (originalFrame - run.First + 1) % run.Count + run.First; if (nextFrame < 0 || nextFrame >= _frames[type].Codes.Count) throw new ArgumentOutOfRangeException(nameof(parent), "경로 다음 프레임");
            }
        }
        if (nextFrame != originalFrame) { _hooks.Unpop!(parent, 0); RawPathSlot.SetFrame(_pool, parent, nextFrame); _hooks.Pop!(parent, sample.Position, 0x40); }
    }
    /// <summary>사제 정지 접두/공통 도착과 삭제 뒤 외부 clear를 원문 PATCH/CD 분기로 실행한다.</summary>
    public void Arrive()
    {
        PathLifeWatch watch = _watch;
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            if (_priest || _worm || _dude) new RawPathAnimation(_pool, _frames, new() { Unpop = _hooks.Unpop, Repop = _hooks.Repop }).Stop(Parent());
            if (!watch.Alive) return;
            // 원문 raw base 부모는 최초 Parent()와 구별해 조회한다.
            int CurrentParent()
            {
                uint value = (uint)Runtime.ReadBits(8); if (value > ushort.MaxValue) throw new ArgumentOutOfRangeException(nameof(Runtime), "도착 현재 부모 물리 SID 폭"); return (ushort)value;
            }
            RawSidPool pool = _pool; Action<int, RawPathPoint, uint, uint> clear = _hooks.ClearBlocked!;
            RawPathArrivalParent.Run(_state.Checking, new()
            {
                Alive = () => watch.Alive, Assertion = _hooks.Assertion, Parent = CurrentParent,
                ValidAlive = parent => parent > 0 && parent < _pool.Capacity && (_pool.Slot(parent)[11] & 3) == 0,
                Step = () => Step(CurrentParent()).Position, Unpop = parent => _hooks.Unpop!(parent, 0), Pop = (parent, point) => _hooks.Pop!(parent, point, 0),
                Robust = () => _ = Robust(), Blocked = Blocked, Halt = Halt, Finished = Runtime.SetFinished,
                DeadOrFree = parent => (_pool.Slot(parent)[11] & 3) != 0, WatchPresent = () => Runtime.DeathWatchToken != 0,
                ArmWatch = () => Runtime.DeathWatchToken = 1, ClearWatch = () => Runtime.DeathWatchToken = 0,
                Arrival = parent => _hooks.Arrival!(parent, Runtime.TargetA, Runtime.TargetB), Kill = () => CurrentHost().Kill(this, 0),
                Clear = parent => { ReadOnlySpan<byte> raw = pool.Slot(parent); clear(parent, RawPathSlot.Position(pool, parent), raw[10], raw[34]); },
            }); return;
        }
        int savedParent = Parent();
        if (_priest || _worm || _dude) new RawPathAnimation(_pool, _frames, new() { Unpop = _hooks.Unpop, Repop = _hooks.Repop }).Stop(savedParent);
        RawPathSample sample = Step(savedParent); _hooks.Unpop!(savedParent, 0); _hooks.Pop!(savedParent, sample.Position, 0); if (!watch.Alive || !Robust()) return;
        bool blocked = Blocked(sample.Position); RawSidPool currentPool = _pool; Action<int, RawPathPoint, uint, uint> clearCurrent = _hooks.ClearBlocked!;
        // 자신이 삭제된 뒤에는 보존한 Pool/함수/부모만 사용한다.
        void ClearSaved()
        {
            ReadOnlySpan<byte> raw = currentPool.Slot(savedParent); clearCurrent(savedParent, RawPathSlot.Position(currentPool, savedParent), raw[10], raw[currentPool.Edition == OriginalEdition.Patch1078 ? 34 : 25]);
        }
        if (Halt()) { if (blocked) ClearSaved(); return; }
        Runtime.SetFinished(true); uint result = _hooks.Arrival!(savedParent, Runtime.TargetA, Runtime.TargetB);
        if (result != 1000 && result != 1001 && result != 1002) throw new InvalidOperationException("경로 도착 반환값");
        if (!watch.Alive) { if (result != 1001) throw new InvalidOperationException("경로 도착 자기 삭제 반환값"); if (blocked) ClearSaved(); return; }
        if (result == 1002) Runtime.SetFinished(false); else CurrentHost().Kill(this, 0); if (blocked) ClearSaved();
    }
}
