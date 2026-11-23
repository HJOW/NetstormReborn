using Netstorm.Core.Original;

// 현재 Gun/부모 TYPE/단계로 원문 파생 공장을 같은 실제 Host에 선택한다.
namespace Netstorm.Client.Mission;

/// <summary>썬 아처·캐논·반더·크로스보우의 실제 생성 부모다.</summary>
public sealed partial class RawCampaignArcherBindings
{
    /// <summary>현재 발사 문맥을 빌리고 다른 Gun 경계는 값 복사로 보존해 실제 파생 생성기를 연결한다.</summary>
    public GunProcessHooks GunHooks(GunProcessHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); _iceContext = hooks.ShotContext; GunProcessHooks result = hooks.Copy();
        Action<uint, ushort, ushort>? fallback = hooks.StartAnimation;
        result.StartAnimation = (stage, form, target) =>
        {
            GunProcess gun = _core.Processes.FromForm(form) as GunProcess ?? throw new InvalidOperationException("캠페인 총 애니메이션의 현재 GunProcess 조회 실패");
            byte type = _core.Pool.Slot(gun.Parent())[10];
            if (type == _ice.Type() && stage == 5) { if (RawIceCannonFire.Create(_core.Processes, _ice, form, target) is null) throw new InvalidOperationException("캠페인 아이스 캐논 발사 부착 실패"); return; }
            if (type == _sunFire.Type() && stage == 5) { if (RawCannonFire.Create(_core.Processes, _sunFire, form, target) is null) throw new InvalidOperationException("캠페인 썬 캐논 발사 부착 실패"); return; }
            if (type == _cannon.SunCannonType() && stage is 2 or 8) { if (RawGunAnimation.Create(_core.Processes, _cannon, stage, form, target) is null) throw new InvalidOperationException("캠페인 썬 캐논 애니메이션 부착 실패"); return; }
            if (type == _thunderFire.Type() && stage == 5) { if (RawCannonFire.Create(_core.Processes, _thunderFire, form, target) is null) throw new InvalidOperationException("캠페인 썬더 캐논 발사 부착 실패"); return; }
            if (type == _vanderFire.Type() && stage == 5) { if (RawCannonFire.Create(_core.Processes, _vanderFire, form, target) is null) throw new InvalidOperationException("캠페인 반더 타워 발사 부착 실패"); return; }
            if (type == _thunderFire.Type() && stage == 2) { if (RawGunAnimation.Create(_core.Processes, _thunderRotate, stage, form, target) is null) throw new InvalidOperationException("캠페인 썬더 캐논 회전 부착 실패"); return; }
            if (type == _crossbow.Type() && stage == 2) { if (RawArcherAnimation.Create(_core.Processes, _crossbowRotate, stage, form, target) is null) throw new InvalidOperationException("캠페인 크로스보우 회전 부착 실패"); return; }
            if (type == _crossbow.Type() && stage == 5) { if (RawCrossbowAnimation.Create(_core.Processes, _crossbow, form, target) is null) throw new InvalidOperationException("캠페인 크로스보우 발사 부착 실패"); return; }
            (fallback ?? throw new InvalidOperationException($"캠페인 총 애니메이션 미복원 TYPE{type} stage{stage}"))(stage, form, target);
        };
        return RawArcherAnimation.MakeGunHooks(_core.Processes, _animation, result);
    }
}
