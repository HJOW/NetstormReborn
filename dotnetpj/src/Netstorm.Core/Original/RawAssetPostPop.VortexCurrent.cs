// 신전 현재 부모의 진단 복귀·공유 목록·Player/섬/내용물 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>Damageable 뒤의 현재 원문 신전 본문과 원소별 포장이다.</summary>
public sealed partial class RawAssetPostPop
{
    /// <summary>signed count/capacity 관문 뒤 현재 count로 중복을 찾고 최초 위치에 저장한다.</summary>
    private static void AppendUniqueAssetList(RawPostPopList list, int sid)
    {
        uint used = list.Count, capacity = list.Capacity;
        if (unchecked((int)used) >= unchecked((int)capacity)) return;
        if (unchecked((int)used) > 0)
        {
            uint[] storage = list.Entries; uint index = 0;
            // 첫 항목은 최초 양수 count로 선택하고 이후 조건은 실제 현재 count를 읽는다.
            do
            {
                if (index >= storage.Length) throw new ArgumentOutOfRangeException(nameof(list), "신전 중복 검색 물리 배열 범위 오류");
                if (storage[index] == (uint)sid) return;
                index++;
            } while (unchecked((int)index) < unchecked((int)list.Count));
        }
        if (unchecked((int)used) < 0) throw new ArgumentOutOfRangeException(nameof(list), "신전 목록 음수 저장 위치");
        uint[] destination = list.Entries;
        if (used >= destination.Length) throw new ArgumentOutOfRangeException(nameof(list), "신전 목록 실제 저장 범위 오류");
        destination[used] = (uint)sid; list.Count = unchecked(list.Count + 1);
    }

    /// <summary>실제 원문 Vortex.cpp 식/행을 전달한다. 복귀하면 현재 물리와 전역을 다시 읽는다.</summary>
    private void VortexAssertion(string expression, uint line) => AssetNeed(_hooks.Assertion)(expression, "\\Ns\\O\\Vortex.cpp", line);

    /// <summary>공통 부모 후 원소별 현재 프레임과 독립 Regular 예약을 수행한다.</summary>
    private void VortexCurrent(int sid, uint flags, uint kind)
    {
        VortexBaseCurrent(sid, flags);
        if ((flags & 1) != 0)
        {
            bool patch = _pool.Edition == OriginalEdition.Patch1078;
            if (kind != VortexWindKind)
            {
                uint frame = AssetNeed(_hooks.TypeDword)(sid, 0x134); Write(_pool.RawSlot(sid), patch ? 36 : 34, frame, patch ? 4 : 1);
            }
            Regular(sid, 1, kind == VortexThunderKind ? 0.1f : 0.08f);
        }
    }

    /// <summary>Damageable→새 Player 기록→음향/WORD→두 목록→섬/권한→실시간 삭제→내용물 순서다.</summary>
    private void VortexBaseCurrent(int sid, uint flags)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078; int ownerOffset = patch ? 34 : 32, extraOffset = patch ? 40 : 35;
        _damageable.DamageablePostPop(sid, flags);
        if ((flags & 2) != 0)
        {
            if (_pool.Slot(sid)[ownerOffset] == 0)
            {
                if (!patch) throw new InvalidOperationException("CD Vortex postPop 새 신전 소유자 assert");
                VortexAssertion("getPlayerId() != INVALID_PLAYER_ID", 0x259);
            }
            uint owner = _pool.Slot(sid)[ownerOffset]; SetPlayerTemple(owner, 2, (uint)sid);
            uint copyOwner = _pool.Slot(sid)[ownerOffset]; SetPlayerTemple(copyOwner, 3, PlayerTemple(copyOwner, 2));
        }
        if ((flags & 1) == 0) return;
        _state.TempleCount = unchecked(_state.TempleCount + 1);
        if ((_pool.Slot(sid)[extraOffset] & 8) == 0)
        {
            uint sound = AssetNeed(_hooks.TypeDword)(sid, 0x74); AssetNeed(_hooks.PlayImpactSound)(sound); SetAssetWord(sid, 0);
            uint owner = _pool.Slot(sid)[ownerOffset];
            if (owner != 0 && owner <= 8 && PlayerTemple(owner, 0) == 0 && (_pool.Slot(sid)[extraOffset] & 1) == 0)
            {
                SetPlayerTemple(owner, 0, (uint)sid); SetPlayerTemple(owner, 1, (uint)sid);
            }
        }
        if ((_pool.Slot(sid)[extraOffset] & 9) == 0)
        {
            AppendUniqueAssetList(_state.CurrentTemples, sid); AppendUniqueAssetList(_state.CurrentResidences, sid);
            if (_state.Selected != 0 && _pool.Slot(sid)[ownerOffset] == LocalOwner())
            {
                if (_state.Selected > ushort.MaxValue) throw new ArgumentOutOfRangeException(nameof(sid), "신전 선택 SID의 물리 범위 오류");
                ReadOnlySpan<byte> form = _pool.Slot((int)_state.Selected); uint type = Read(form, 18, 2);
                if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "신전 선택 Form의 현재 TYPE 물리 범위 오류");
                if ((_types[(int)type].Flags2 & 0x200) != 0) AssetNeed(_hooks.ClearPlacement)();
            }
            uint frame = AssetNeed(_hooks.TypeDword)(sid, 0x130); Write(_pool.RawSlot(sid), patch ? 36 : 34, frame, patch ? 4 : 1);
            ReadOnlySpan<byte> raw = _pool.Slot(sid); int island = AssetNeed(_hooks.IslandNumber)(X(raw, 14), X(raw, 18));
            if (!AssetNeed(_hooks.IslandExists)(island))
            {
                if (!patch) throw new InvalidOperationException("CD Vortex postPop islandList.exists assert");
                VortexAssertion("islandList.exists( islandId )", 0x281);
            }
            if (!patch && AssetNeed(_hooks.IslandTemple)(island) != 0) throw new InvalidOperationException("CD Vortex postPop 기존 신전 assert");
            AssetNeed(_hooks.RegisterTemple)(island, sid); AssetNeed(_hooks.SetOwner)(sid, _pool.Slot(sid)[ownerOffset]);
            if (_state.CurrentBoss)
            {
                uint current = 2;
                // 최초 살아 있는 자산/Form을 같은 실제 풀에서 찾는다.
                while (current < _pool.Capacity && (_pool.Slot((int)current)[11] & 1) != 0) current++;
                // 각 삭제 이전에 다음 살아 있는 SID를 고정하여 효과로 free가 되어도 다음 후보를 유지한다.
                while (current < _pool.Capacity)
                {
                    int other = (int)current; uint next = current + 1;
                    // 현재 가상 삭제가 바꾸기 전에 다음 번호를 확보한다.
                    while (next < _pool.Capacity && (_pool.Slot((int)next)[11] & 1) != 0) next++;
                    ReadOnlySpan<byte> otherRaw = _pool.Slot(other);
                    if ((otherRaw[extraOffset] & 8) == 0 && (Type(other).Flags2 & 0x200) != 0 && other != sid)
                    {
                        float x = X(otherRaw, 14), y = X(otherRaw, 18);
                        if (!patch) { x = unchecked((int)Truncate(x)) / 16; y = unchecked((int)Truncate(y)) / 16; }
                        if (AssetNeed(_hooks.RegionNumber)(x, y) == island) AssetNeed(_hooks.Destroy)(other, 0x100000);
                    }
                    current = next;
                }
            }
        }
        if (_state.LoadingDepth == 0 && (_pool.Slot(sid)[extraOffset] & 9) == 0)
        {
            if (!AssetNeed(_hooks.HasContent)(sid, _state.SlotType)) AssetNeed(_hooks.CreateSlot)(sid, _state.SlotType);
            AssetNeed(_hooks.RefreshProduction)();
        }
    }
}
