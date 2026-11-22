// 아처의각속도/각가속도갱신과출력/실제투사체효과순서를보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawArcherAnimation.cpp의부착후Fire생성·Launch/Fire 본체다.</summary>
public sealed partial class RawArcherAnimation
{
    /// <summary>실제부착뒤현재프레임으로각상태를준비하고출력WORD bit0만끄고통지한다.</summary>
    public void InitializeFire(ArcherAnimationFields fields)
    {
        _ = Parent(fields); Action<ushort> notify = Need(_hooks.NotifySurface); F(fields.Bytes, 64, InitialAngle(Frame(fields.Parent()))); F(fields.Bytes, 68, _state.InitialSpeed); F(fields.Bytes, 72, _state.InitialAcceleration); Put(fields.Bytes, 76, 0); Put(fields.Bytes, 80, 0); Pool.AllocatedBytes(fields.Parent())[12] &= 0xfe; notify(fields.Parent());
    }
    /// <summary>현재HP표적에서만속도35/표시83/효과0의실제Missile생성을요구한다.</summary>
    public bool Launch(ArcherAnimationFields fields)
    {
        _ = Parent(fields); ushort sid = fields.Target(); if (sid == 0 || sid >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields)); ReadOnlySpan<byte> raw = Pool.Slot(sid); if (raw[10] >= _types.Count) throw new ArgumentOutOfRangeException(nameof(fields)); if ((raw[11] & 3) != 0 || (_types[raw[10]].Flags1 & 0x10) == 0) return false;
        Func<ushort, ushort, float[]> point = Need(_hooks.Common.TargetPoint); Action<GunProjectileRequest> create = Need(_hooks.Common.CreateProjectile); var request = new GunProjectileRequest { Firing = fields.Parent(), Target = sid, Speed = 35, DisplayType = _state.DisplayType, EffectType = 0, TargetPosition = Point(fields.Bytes, 12) }; request.TargetCenter = point(fields.Parent(), sid).ToArray(); create(request); return true;
    }
    /// <summary>매틱원문float각상태를저장하고spring/출력/발사/그림/완료순서를실행한다.</summary>
    public void Fire(ArcherAnimationFields fields)
    {
        if (fields.Target() == 0) throw new InvalidOperationException("아처 Fire 표적0 원문관문");
        if (_state.Delta > 0 && _state.Delta < 2)
        {
            ReadOnlySpan<byte> raw = Parent(fields); bool alive = Track(fields); double delta = (float)_state.Delta; F(fields.Bytes, 64, (float)(delta * F(fields.Bytes, 68) + F(fields.Bytes, 64))); F(fields.Bytes, 68, (float)(delta * F(fields.Bytes, 72) + F(fields.Bytes, 68))); float threshold = (float)((double)_state.Winding * 360 + Degrees(Point(raw, 14), Point(fields.Bytes, 20)) - _state.FireBias), position = F(fields.Bytes, 64);
            if (Read(fields.Bytes, 80) == 0)
            {
                if (Read(fields.Bytes, 76) == 0 && threshold - _state.SpringOffset <= position) { Action<ushort, string> sound = Need(_hooks.Common.Sound); Put(fields.Bytes, 76, 1); F(fields.Bytes, 72, -F(fields.Bytes, 72)); sound(fields.Parent(), "sasprung.wav"); }
                if (Read(fields.Bytes, 80) == 0 && threshold <= position)
                {
                    Action<ushort> notify = Need(_hooks.NotifySurface); Action<ushort, string> sound = Need(_hooks.Common.Sound); Put(fields.Bytes, 76, 1); Put(fields.Bytes, 80, 1); Pool.AllocatedBytes(fields.Parent())[12] |= 1; notify(fields.Parent()); if (!Launch(fields) && alive) return; sound(fields.Parent(), "safired.wav");
                }
            }
            if (Read(fields.Bytes, 80) != 0 && Read(fields.Bytes, 76) != 0 && threshold + _state.ResetOffset <= position) Put(fields.Bytes, 76, 0);
            if (position > 0)
            {
                int next = FireFrame(position, Read(fields.Bytes, 76) != 0); if (next != Frame(fields.Parent())) { SetFrame(fields.Parent(), next); if (F(fields.Bytes, 68) < 0) Need(_hooks.Common.Sound)(fields.Parent(), "sarachet.wav"); else if (Frame(fields.Parent()) == 2) Need(_hooks.Common.Sound)(fields.Parent(), "sunDiscThrowerWhoosh.wav"); }
            }
        }
        float speed = F(fields.Bytes, 68); bool completed = false;
        if (Read(fields.Bytes, 80) != 0 && speed <= _state.FinishSpeed) { double angle = RawBoardAngle.Normalize(F(fields.Bytes, 64), Pool.Edition, _state.Angles); completed = Math.Abs(angle - _state.FinishAngle) < _state.FinishTolerance; }
        if (completed || F(fields.Bytes, 64) < 0) Finish(fields, 6, 7);
    }
}
