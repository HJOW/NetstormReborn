using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 별도 소유/외부 전역 버퍼와 실제 표시 조회의 수명 계약이다.

/// <summary>cpppj StyleTextBufferStore의 실제 상태/소멸/free 순서를 검사한다.미완료 꼬리를 임의 반납하지 않는다.</summary>
public sealed class StyleTextBufferStoreTests
{
    /// <summary>외부 상태 자체를 빌리고 조각 소멸 뒤 free 통지까지 레코드를 유지한다.</summary>
    [Fact]
    public void BorrowedIdentityDestructionReleaseAndUnfinishedTail()
    {
        var s = new StyleBufferScene(); var external = new StyleTextBufferState(); var released = new List<string>(); StyleTextBufferStore? store = null; store = new(s.Fragments, s.Style, s.BufferHooks, t => { Assert.Same(external, store!.Binding(t).State); Assert.False(external.Initialized); released.Add("free:" + t); }); store.ConstructBorrowed(10, external, 8); var binding = store.Binding(10); Assert.Same(external, binding.Handler.State); binding.Handler.AppendText("A\nTAIL\0"u8, s.Context); binding.Handler.Layout(99); uint tail = external.HardLines[1]; s.StyleHooks.Release = t => released.Add("fragment:" + t); store.Destroy(10); Assert.Equal(new[] { "fragment:1", "fragment:3", "free:10" }, released); Assert.Single(s.Fragments.Fragments); Assert.True(s.Fragments.Fragments.ContainsKey(tail)); Assert.Throws<InvalidOperationException>(() => store.Binding(10)); s.Style.Destroy(tail, 1);
    }
    /// <summary>생성의 훅 사본과 실제 owned 상태를 표시 조회에 연결하며 Gump 등록 수는 늘리지 않는다.</summary>
    [Fact]
    public void OwnedBufferBindingAndCopiedHooksUseActualDraw()
    {
        var s = new StyleBufferScene(); var store = new StyleTextBufferStore(s.Fragments, s.Style, s.BufferHooks, _ => { }); uint count = s.Create.Count, serial = s.Create.Serial; store.Construct(10, 8); Assert.Equal((count, serial), (s.Create.Count, s.Create.Serial)); StyleTextBufferStore.BindScrollingTextBuffers(s.ScrollHooks, store); s.BufferHooks.MeasureText = (_, _, _) => throw new InvalidOperationException("생성 뒤 외부 훅 변경은 사본에 전파되지 않음"); s.Scroll.AppendText(2, "A\n\0"u8); s.Draw.Draw(2); var binding = store.Binding(10); Assert.True(binding.State.Initialized); Assert.NotSame(s.State, binding.State); Assert.Equal(171, s.Pixels.Span[34]); store.Destroy(10); Assert.Empty(s.Fragments.Fragments);
    }
    /// <summary>중복 토큰/같은 물리 상태의 중복 생성과 실패한 생성의 유령 레코드를 거부한다.</summary>
    [Fact]
    public void TokensPhysicalDuplicatesAndFailedConstruction()
    {
        var s = new StyleBufferScene(); var store = new StyleTextBufferStore(s.Fragments, s.Style, s.BufferHooks, _ => { }); Assert.Throws<ArgumentException>(() => store.Construct(0, 8)); Assert.Throws<ArgumentException>(() => store.Construct(10, 0)); Assert.Throws<InvalidOperationException>(() => store.Binding(10)); store.Construct(10, 8); Assert.Throws<ArgumentException>(() => store.Construct(10, 8)); Assert.Throws<ArgumentException>(() => store.ConstructBorrowed(11, store.Binding(10).State, 8)); Assert.Throws<InvalidOperationException>(() => store.Binding(11)); store.Destroy(10); Assert.Equal(10U, store.Construct(10, 8));
    }
    /// <summary>free 콜백 실패는 이미 소멸한 상태와 등록 레코드를 남기며 소멸을 다시 실행하지 않는다.</summary>
    [Fact]
    public void FreeFailurePreservesPartiallyDestroyedRecord()
    {
        var s = new StyleBufferScene(); int calls = 0; var store = new StyleTextBufferStore(s.Fragments, s.Style, s.BufferHooks, _ => { calls++; throw new InvalidOperationException("의도한 free 경계 실패"); }); store.Construct(10, 8); Assert.Throws<InvalidOperationException>(() => store.Destroy(10)); Assert.False(store.Binding(10).State.Initialized); Assert.Throws<ArgumentException>(() => store.Destroy(10)); Assert.Equal(1, calls);
    }
}
