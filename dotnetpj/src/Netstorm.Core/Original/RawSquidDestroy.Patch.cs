using System.Buffers.Binary;

// 실제PATCH가선택하는원문진단/현재가상·체인/반납을독립typed선검사와구별한다.
namespace Netstorm.Core.Original;

/// <summary>명시진단을연결한현재PATCH 공통삭제의dead선기록/복귀단언 경로다.</summary>
public sealed partial class RawSquidDestroy
{
    /// <summary>원문client SID5..14999를 현재풀배치에 대응한다. 예약번호는포함하지않는다.</summary>
    private static bool IsClientSid(int sid) => sid >= FirstOrdinary && sid < RawSidPool.ServerFirst;

    /// <summary>004af780의 실제진입이다. 장치는선택할때만요구하고매효과뒤현재필드를다시읽는다.</summary>
    private void DestroyPatch(int sid, uint flags, SquidDestroyHooks hooks)
    {
        byte initial = _pool.Slot(sid)[StateOffset];
        if ((initial & DeadBit) != 0) return;
        _pool.RawSlot(sid)[StateOffset] = (byte)(initial | DeadBit);
        if ((initial & FreeBit) != 0) AssertPatch("!isFree()", 0x24d);
        if ((_pool.Slot(sid)[StateOffset] & FreeBit) != 0) return;
        if (_pool.IsServer)
        {
            if (IsClientSid(sid)) flags |= ClientFlag;
        }
        else if ((flags & ServerFlag) == 0)
        {
            if (!IsClientSid(sid)) AssertPatch("isClientSid()", 0x269);
            if (!IsClientSid(sid)) return;
        }
        uint pre = _preDepth++; int type = _pool.Slot(sid)[TypeOffset];
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "PATCH 공통 삭제의 현재 로그 TYPE 범위 오류");
        string name = _types[type].Name; int zero = name.IndexOf('\0'); if (zero >= 0) name = name[..zero];
        string text = $"destroying {name} {sid}\n";
        Action<int, string> trace = _patchDiagnostics?.Trace ?? throw new InvalidOperationException("PATCH 공통 삭제의 현재 로그 장치 미연결");
        trace(0x27, text);
        // 진단/로그가현재콜백을교체해도각가상호출직전에선택한함수의수명을보존한다.
        void Emit(SquidDestroyEvent effect)
        {
            Action<SquidDestroyEvent> current = hooks.Emit ?? throw new InvalidOperationException("PATCH 공통 삭제의 현재 가상 효과 미연결");
            current(effect);
        }
        Emit(new(SquidDestroyEffect.PreDestroy, sid, 0, flags));
        if (_preDepth != pre) AssertPatch("baseDestroy == curDestroy", 0x276);
        int dependent = Word(sid, HeadOffset); var visited = new HashSet<int>();
        // 전체체인선검사없이현재머리부터처리하며next는ReleaseDependent전에보존한다.
        while (dependent != 0)
        {
            if (!visited.Add(dependent)) throw new InvalidOperationException("PATCH 공통 삭제의 종속 순환은 호스트에서 지원하지 않습니다");
            if ((_pool.Slot(dependent)[StateOffset] & FreeBit) != 0) AssertPatch("!s.isFree()", 0x27d);
            if ((_pool.Slot(dependent)[StateOffset] & ContainedBit) == 0 && _pool.Slot(dependent)[TypeOffset] >= OriginalConstructors.FirstAssetType)
                AssertPatch("s.isContained() || s.isForm()", 0x27e);
            bool free = (_pool.Slot(dependent)[StateOffset] & FreeBit) != 0; int next = Word(dependent, NextOffset);
            if (free) AssertPatch("!s.isFree()", 0x280);
            Emit(new(SquidDestroyEffect.ReleaseDependent, dependent, sid, 0));
            if ((_pool.Slot(dependent)[StateOffset] & FreeBit) == 0) Emit(new(SquidDestroyEffect.DestroyDependent, dependent, sid, flags | DependentFlag));
            dependent = next;
        }
        if ((_pool.Slot(sid)[StateOffset] & FreeBit) != 0) AssertPatch("!isFree()", 0x28f);
        if ((_pool.Slot(sid)[StateOffset] & FreeBit) != 0) return;
        if ((flags & (ClientFlag | DependentFlag)) == 0 && _pool.IsServer && !IsClientSid(sid))
            Emit(new(SquidDestroyEffect.Transmit, sid, 0, flags | ServerFlag));
        Func<int>? selected = hooks.Selected; Func<uint>? selectedRaw = hooks.SelectedRaw;
        if (selectedRaw is null && selected is null) throw new InvalidOperationException("PATCH 공통 삭제의 현재 선택 조회 미연결");
        if ((selectedRaw is not null ? selectedRaw() : unchecked((uint)selected!())) == (uint)sid)
            Emit(new(SquidDestroyEffect.ClearSelection, sid, 0, 0));
        if (sid == 0 || sid >= _pool.Capacity) AssertPatch("isValid()", 0x2a2);
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if ((raw[StateOffset] & VoidBit) == 0)
        {
            uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw); bool form = false;
            // raw TYPE가아닌네실제Form VT만현재가상+48의Form장치에분배한다.
            foreach (uint formType in new uint[] { 1, 2, 3, 6 }) if (table == RawSquidForms.VtableFor(formType)) { form = true; break; }
            if (form)
            {
                Action<int> unpop = hooks.UnpopForm ?? throw new InvalidOperationException("PATCH 공통 삭제의 현재 Form Unpop 미연결");
                unpop(sid);
            }
            else
            {
                if (!RawSquidFactory.SupportsConstructedVtable(_pool.Edition, table)) throw new InvalidOperationException("PATCH 공통 삭제의 현재 가상+48 미지 VT");
                if (raw[TypeOffset] >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "PATCH 공통 삭제의 현재 Unpop TYPE 범위 오류");
                _unpop.Unpop(sid, _types[raw[TypeOffset]], 0);
            }
        }
        uint post = _postDepth++; Emit(new(SquidDestroyEffect.PostDestroy, sid, 0, flags));
        if (_postDepth != post) AssertPatch("curPostDestroy == basePostDestroy", 0x2a9);
        _pool.ReleasePatch(sid, (expression, file, line) => AssertPatch(expression, line, file));
    }
}
