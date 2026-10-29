using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 메뉴 객체 부착/지연 수명을 공용 SID 풀의 GumpForm에 연결한다.

/// <summary>cpppj/src/client/GumpFormBinding.cpp의 현재 등록표 알림과 실제 같은 풀/메뉴/Form 연결.</summary>
public static class GumpFormBinding
{
    /// <summary>원본 WORD SID 표현 범위를 확인하고 조용한 절단 없이 전달한다.</summary>
    private static int Sid(uint value) => value <= ushort.MaxValue ? (int)value : throw new ArgumentOutOfRangeException(nameof(value), "GumpForm SID 표현 범위 초과");
    /// <summary>알림 때마다 현재 실제 등록 ID→Gump 토큰을 조회한다.소유0 생략은 Form.Notify의 책임이다.</summary>
    public static GumpFormNotice MakeGumpFormNotice(GumpCreateState state, Action<uint, int> releaseObject)
    { ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(releaseObject); return (owner, parent) => { if (owner >= state.Entries.Length) throw new ArgumentOutOfRangeException(nameof(owner)); uint token = state.Entries[owner]; if (token == 0) throw new InvalidOperationException("GumpForm 소유 UI 등록 없음"); releaseObject(token, parent); }; }
    /// <summary>기본 객체 삭제 통지는 부모 SID를 쓰지 않고 실제 자식 우선 Kill을 수행한다.</summary>
    public static void ReleaseGumpObject(GumpLifetime lifetime, uint token, int objectId) => lifetime.Kill(token);
    /// <summary>메뉴 Form(type3,2)/현재 소유 ID/객체 부착을 실제 같은 SID 풀에 연결한다.월드 Form 분배 수명은 호출자가 소유한다.</summary>
    public static void BindMenuGumpForms(GumpCreateState create, MenuGumpWidgetState state, MenuGumpWidgetHooks hooks, RawGumpFormHost forms)
    {
        ArgumentNullException.ThrowIfNull(forms); if (create.Collection.Edition != forms.Pool.Edition) throw new ArgumentException("메뉴와 GumpForm 판본 불일치"); state.ObjectFormType = RawGumpFormHost.GumpFormType;
        hooks.ObjectForm = (type, flags) => { if (type != RawGumpFormHost.GumpFormType) throw new ArgumentException("메뉴 객체 GumpForm 타입 오류"); uint form = unchecked((uint)forms.Create(flags)); return new(form, form); }; hooks.FormOwner = (form, owner) => forms.SetOwner(Sid(form), owner); hooks.FormObject = (form, obj, flags) => forms.Attach(Sid(form), Sid(obj), flags);
    }
    /// <summary>Gump Kill의 실제 Form 인덱스를 공통 삭제/Unpop/반납에 연결한다.다른 Form의 기존 삭제는 보존한다.</summary>
    public static void BindGumpFormLifetime(GumpLifetimeHooks hooks, RawGumpFormHost forms)
    { ArgumentNullException.ThrowIfNull(forms); var previous = hooks.DeleteForm; hooks.DeleteForm = (form, flags) => { int sid = Sid(form); if (forms.IsGumpForm(sid)) forms.Destroy(sid, flags); else (previous ?? throw new ArgumentException("GumpForm 외 UI Form 삭제 경계 필요"))(form, flags); }; }
}
