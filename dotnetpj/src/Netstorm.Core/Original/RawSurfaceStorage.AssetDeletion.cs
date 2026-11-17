// 현재 자산의 실제 VT+14/+18/+34를 같은 장기 삭제 부모에 연결한다.
// cpppj/src/client/RawSurfaceAssetDeletion.cpp 의 표·BindRegionLifecycle/NotifyAssetDependency 와
// RawSurfaceWorld.h 의 EmitDefaultAssetEffect 장기 본체를 이식한다.
// 0.573에서 추가됐다. (R4-1 잔여 중 파생 전용 삭제 라우팅 골격, 개별 몸체 연결은 다음 단위)
namespace Netstorm.Core.Original;

/// <summary>현재 raw 슬롯의 VT DWORD와 원본 VT+14/+18/+34 진입점 묶음이다.</summary>
public readonly record struct NativeAssetDeletionEntry(uint Table, uint Pre, uint Post, uint Notice);

/// <summary>World 없이 같은 장기 Pool/전역으로 자산 삭제 효과를 분배한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>장기 다이스·제단 고유 삭제의 실제 호스트 장치 묶음이다. 0.593에서 추가됐다.</summary>
    public sealed class RawSurfaceDaisLifecycle
    {
        /// <summary>0x0044a4d0 제단 전처리다. 다이스 가상 삭제와 추상 룬 정리는 본체가 맡는다. 0.593에서 추가됐다.</summary>
        public Action<int, uint>? AltarPre { get; set; }

        /// <summary>0x0044afd0 다이스 전처리다. 제단 완료와 사제 중지는 본체가 맡는다. 0.593에서 추가됐다.</summary>
        public Action<int, uint>? DaisPre { get; set; }

        /// <summary>0x00449580 다이스 후처리다. Player SID 쌍 정리는 본체가 맡는다. 0.593에서 추가됐다.</summary>
        public Action<int, uint>? DaisPost { get; set; }

        /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.593에서 추가됐다.</summary>
        internal RawSurfaceDaisLifecycle Copy() => new()
        {
            AltarPre = AltarPre,
            DaisPre = DaisPre,
            DaisPost = DaisPost,
        };
    }

    /// <summary>보호 PE에서 읽은 83개 생성자 VT의 고정 라우팅표다. 주소는 가상 선택용 정수이며 호스트 함수로 실행하지 않는다. 0.573에서 추가됐다.</summary>
    private static readonly NativeAssetDeletionEntry[] NativeAssetDeletionEntries =
    [
        new(0x00501dd0u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00501ea0u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x00502370u, 0x0041e960u, 0x004b0840u, 0x0041c100u),
        new(0x00502448u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00502510u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x005025d8u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x005026a0u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00502778u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00502840u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00502908u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x005029d0u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00502aa8u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00502b70u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00502c38u, 0x0041f910u, 0x004b0840u, 0x0041c100u),
        new(0x00502cf0u, 0x0041f910u, 0x004b0840u, 0x0041c100u),
        new(0x00502db0u, 0x0041f910u, 0x004b0840u, 0x0041c100u),
        new(0x00502e68u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00502f30u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00502ff8u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x005030c0u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00503188u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00503360u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x005034c8u, 0x004221b0u, 0x00422300u, 0x0041c100u),
        new(0x00503d80u, 0x00426890u, 0x0044b9e0u, 0x0041c100u),
        new(0x00503e50u, 0x00426890u, 0x0044b9e0u, 0x0041c100u),
        new(0x00503f20u, 0x00426890u, 0x0044b9e0u, 0x0041c100u),
        new(0x00503ff0u, 0x00426890u, 0x0044b9e0u, 0x0041c100u),
        new(0x005040c0u, 0x0042ae80u, 0x0044b9e0u, 0x0041c100u),
        new(0x00504198u, 0x0042ae80u, 0x0044b9e0u, 0x0041c100u),
        new(0x00506d98u, 0x00442240u, 0x004b0840u, 0x0041c100u),
        new(0x00506e50u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00506f08u, 0x004b0950u, 0x00442640u, 0x0041c100u),
        new(0x005070c0u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x005075d8u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x005076b8u, 0x0044a4d0u, 0x00448b60u, 0x0041c100u),
        new(0x00507770u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00507950u, 0x0044afd0u, 0x00449580u, 0x0041c100u),
        new(0x00508cc8u, 0x004b0950u, 0x00450920u, 0x0041c100u),
        new(0x00509148u, 0x0044b4b0u, 0x0044b9e0u, 0x0048f0a0u),
        new(0x00509210u, 0x00455840u, 0x0044b9e0u, 0x0048f0a0u),
        new(0x005092d0u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00509388u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00509470u, 0x00457990u, 0x0044b9e0u, 0x0041c100u),
        new(0x005096a0u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00509758u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00509810u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x00509930u, 0x0044b4b0u, 0x0044b9e0u, 0x0045b4a0u),
        new(0x005099e8u, 0x0044b4b0u, 0x0044b9e0u, 0x0045b5b0u),
        new(0x00509aa0u, 0x0044b4b0u, 0x0044b9e0u, 0x0045b680u),
        new(0x00509b98u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x0050a050u, 0x0045e7e0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050a1e8u, 0x0045e7e0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050a388u, 0x0045e7e0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050a748u, 0x0041f910u, 0x004b0840u, 0x0041c100u),
        new(0x0050a820u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050a8e8u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050b570u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x0050b6e8u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050b7c0u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050b888u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050b9b8u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050bad0u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050c3b0u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x0050c9f0u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x0050cc08u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x0050da88u, 0x00485580u, 0x00485540u, 0x0041c100u),
        new(0x0050db78u, 0x004b0950u, 0x004b0840u, 0x0041c100u),
        new(0x0050dc30u, 0x00484f80u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050e568u, 0x00426890u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050e668u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050e738u, 0x0044b4b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050e888u, 0x004b0950u, 0x0048eb80u, 0x0041c100u),
        new(0x0050e940u, 0x0044b4b0u, 0x0048edf0u, 0x0041c100u),
        new(0x0050ea08u, 0x0044b4b0u, 0x0048edf0u, 0x0041c100u),
        new(0x0050ead0u, 0x0044b4b0u, 0x0048edf0u, 0x0041c100u),
        new(0x0050eb98u, 0x004b0950u, 0x004b0840u, 0x0048f0a0u),
        new(0x0050ee08u, 0x004906c0u, 0x004b0840u, 0x0041c100u),
        new(0x0050f210u, 0x004919b0u, 0x0044b9e0u, 0x0041c100u),
        new(0x0050f540u, 0x004b0950u, 0x0049a4a0u, 0x0041c100u),
        new(0x005196f0u, 0x004dca50u, 0x0044b9e0u, 0x0041c100u),
        new(0x005197a8u, 0x004dca50u, 0x0044b9e0u, 0x0041c100u),
        new(0x00519860u, 0x004dca50u, 0x0044b9e0u, 0x0041c100u),
        new(0x00519918u, 0x004dca50u, 0x0044b9e0u, 0x0041c100u),
    ];

    /// <summary>장기 신전의 실제 가상 소유자를 선택하는 장치다. 0.573에서 추가됐다.</summary>
    private Action<int, uint>? _vortexOwner;

    /// <summary>장기 신전의 실제 삭제 전처리를 선택하는 장치다. 0.573에서 추가됐다.</summary>
    private Action<int, uint>? _vortexPre;

    /// <summary>장기 Outpost가 두 목록 제거 뒤 선택하는 실제 영역 갱신 장치다. 0.573에서 추가됐다.</summary>
    private Action<float, float>? _regionChanged;

    /// <summary>장기 다이스·제단 고유 삭제의 실제 호스트 장치다. 본체 이식 전까지 HostSession이 공급한다. 0.593에서 추가됐다.</summary>
    private RawSurfaceDaisLifecycle _daisDevices = new();

    /// <summary>장기 Player 고유 전처리의 실제 호스트 장치다. 본체 이식 전까지 HostSession이 공급한다. 0.593에서 추가됐다.</summary>
    private Action<int, uint>? _playerPre;

    /// <summary>장기 비행장 홈 통지의 실제 호스트 장치다. 본체 이식 전까지 HostSession이 공급한다. 0.593에서 추가됐다.</summary>
    private Action<int, uint>? _aviaryHome;

    /// <summary>최초 메뉴에서 약한 실제 장치만 연결하고 이미 선택된 함수의 수명은 각 호출이 보유한다. 0.573에서 추가됐다.</summary>
    /// <param name="vortexOwner">장기 신전의 실제 가상 소유자 장치다.</param>
    /// <param name="vortexPre">장기 신전의 실제 삭제 전처리 장치다.</param>
    /// <param name="regionChanged">장기 Outpost의 실제 영역 갱신 장치다.</param>
    public void BindRegionLifecycle(Action<int, uint> vortexOwner, Action<int, uint> vortexPre, Action<float, float> regionChanged)
    {
        ArgumentNullException.ThrowIfNull(vortexOwner);
        ArgumentNullException.ThrowIfNull(vortexPre);
        ArgumentNullException.ThrowIfNull(regionChanged);
        // Native 전송 뒤, 첫 대여 전에만 실제 장기 가상 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _vortexOwner is not null || _vortexPre is not null || _regionChanged is not null)
        {
            throw new ArgumentException("장기 영역의 최초 Native 가상 장치 중복/누락", nameof(vortexOwner));
        }

        _vortexOwner = vortexOwner;
        _vortexPre = vortexPre;
        _regionChanged = regionChanged;
    }

    /// <summary>첫 다이스·제단 고유 삭제 전에 실제 호스트 장치 묶음만 연결한다. 0.593에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 다이스·제단 호스트 장치 묶음이다.</param>
    public void BindDaisLifecycle(RawSurfaceDaisLifecycle devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // Native 전송 뒤, 첫 대여 전에만 실제 호스트 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed
            || _daisDevices.AltarPre is not null || _daisDevices.DaisPre is not null || _daisDevices.DaisPost is not null
            || devices.AltarPre is null || devices.DaisPre is null || devices.DaisPost is null)
        {
            throw new ArgumentException("장기 다이스·제단의 실제 최초 호스트 장치 중복/누락", nameof(devices));
        }

        _daisDevices = devices.Copy();
    }

    /// <summary>첫 Player 고유 전처리 전에 실제 호스트 장치만 연결한다. 0.593에서 추가됐다.</summary>
    /// <param name="playerPre">설치할 실제 장기 Player 전처리 장치다.</param>
    public void BindPlayerPre(Action<int, uint> playerPre)
    {
        ArgumentNullException.ThrowIfNull(playerPre);
        // Native 전송 뒤, 첫 대여 전에만 실제 호스트 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _playerPre is not null)
        {
            throw new ArgumentException("장기 Player 전처리의 실제 최초 호스트 장치 중복/누락", nameof(playerPre));
        }

        _playerPre = playerPre;
    }

    /// <summary>첫 비행장 홈 통지 전에 실제 호스트 장치만 연결한다. 0.593에서 추가됐다.</summary>
    /// <param name="aviaryHome">설치할 실제 장기 비행장 홈 통지 장치다. 통지 주소를 함께 받는다.</param>
    public void BindAviaryHome(Action<int, uint> aviaryHome)
    {
        ArgumentNullException.ThrowIfNull(aviaryHome);
        // Native 전송 뒤, 첫 대여 전에만 실제 호스트 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _aviaryHome is not null)
        {
            throw new ArgumentException("장기 비행장 홈 통지의 실제 최초 호스트 장치 중복/누락", nameof(aviaryHome));
        }

        _aviaryHome = aviaryHome;
    }

    /// <summary>0x0044a4d0 제단 전처리를 실제 호스트 장치에 분배한다. 본체 이식 뒤에는 RawDaisDeletion에 잇는다. 0.593에서 추가됐다.</summary>
    /// <param name="sid">전처리할 슬롯 번호.</param>
    /// <param name="flags">전처리 플래그.</param>
    public void DaisAltarPre(int sid, uint flags) =>
        (_daisDevices.AltarPre ?? throw new InvalidOperationException("장기 다이스·제단의 실제 제단 전처리 미결합: 0x0044A4D0"))(sid, flags);

    /// <summary>0x0044afd0 다이스 전처리를 실제 호스트 장치에 분배한다. 본체 이식 뒤에는 RawDaisDeletion에 잇는다. 0.593에서 추가됐다.</summary>
    /// <param name="sid">전처리할 슬롯 번호.</param>
    /// <param name="flags">전처리 플래그.</param>
    public void DaisPreDestroy(int sid, uint flags) =>
        (_daisDevices.DaisPre ?? throw new InvalidOperationException("장기 다이스·제단의 실제 다이스 전처리 미결합: 0x0044AFD0"))(sid, flags);

    /// <summary>0x00449580 다이스 후처리를 실제 호스트 장치에 분배한다. 본체 이식 뒤에는 RawDaisDeletion에 잇는다. 0.593에서 추가됐다.</summary>
    /// <param name="sid">후처리할 슬롯 번호.</param>
    /// <param name="flags">후처리 플래그.</param>
    public void DaisPostDestroy(int sid, uint flags) =>
        (_daisDevices.DaisPost ?? throw new InvalidOperationException("장기 다이스·제단의 실제 다이스 후처리 미결합: 0x00449580"))(sid, flags);

    /// <summary>0x004906c0 Player 전처리를 실제 호스트 장치에 분배한다. 본체 이식 뒤에는 RawPlayerWorld에 잇는다. 0.593에서 추가됐다.</summary>
    /// <param name="sid">전처리할 슬롯 번호.</param>
    /// <param name="flags">전처리 플래그.</param>
    public void PlayerPreDestroy(int sid, uint flags) =>
        (_playerPre ?? throw new InvalidOperationException("장기 Player 전처리의 실제 호스트 장치 미결합: 0x004906C0"))(sid, flags);

    /// <summary>실제 가상34의 세 홈 통지를 실제 호스트 장치에 분배한다. 본체 이식 뒤에는 NotifyAviaryHome에 잇는다. 0.593에서 추가됐다.</summary>
    /// <param name="home">홈 자산 번호.</param>
    /// <param name="method">가상34 통지 주소(0x0045b4a0·0x0045b5b0·0x0045b680).</param>
    public void NotifyAviaryHome(int home, uint method) =>
        (_aviaryHome ?? throw new InvalidOperationException($"장기 비행장 홈 통지의 실제 호스트 장치 미결합: 0x{method:X8}"))(home, method);

    /// <summary>현재 83자산의 통지만 소비하며 다른 Form은 기존 실제 Host 분배로 남긴다. 0.573에서 추가됐다.</summary>
    /// <param name="receiver">통지를 받는 자산 번호다.</param>
    /// <param name="parent">통지의 주인 번호다.</param>
    /// <param name="value">통지 값이다.</param>
    public bool NotifyAssetDependency(int receiver, int parent, uint value)
    {
        if (!UsesNativeTransmissionDevices())
        {
            throw new InvalidOperationException("장기 자산 통지의 Native 판본 계약");
        }

        // TYPE/owner/상태를 읽지 않고 실제 현재 가상표만 선택한다.
        ReadOnlySpan<byte> raw = Pool.Slot(receiver);
        uint table = (uint)(raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24));
        foreach (NativeAssetDeletionEntry entry in NativeAssetDeletionEntries)
        {
            if (entry.Table == table)
            {
                EmitDefaultAssetEffect(new SquidDestroyEvent(SquidDestroyEffect.ReleaseDependent, receiver, parent, value));
                return true;
            }
        }

        return false;
    }

    /// <summary>효과를 호출하는 시점의 VT를 읽으며 TYPE/owner/상태로 가상 선택을 바꾸지 않는다. 0.573에서 추가됐다.</summary>
    /// <param name="sid">조회할 객체 번호다.</param>
    private NativeAssetDeletionEntry CurrentAssetDeletionEntry(int sid)
    {
        ReadOnlySpan<byte> raw = Pool.Slot(sid);
        uint table = (uint)(raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24));
        // 실제 생성자 83개만 허용한다.
        foreach (NativeAssetDeletionEntry entry in NativeAssetDeletionEntries)
        {
            if (entry.Table == table)
            {
                return entry;
            }
        }

        throw new InvalidOperationException($"장기 자산 삭제의 미지 VT: 0x{table:X8}");
    }
}
