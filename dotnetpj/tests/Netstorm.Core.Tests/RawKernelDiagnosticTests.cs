// 커널의 단언 복귀·소진 부분 효과를 직접 대조한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 커널 진단을 검증하는 테스트 클래스다. 2026-11-07 F2d 커널 이식에서 추가됐다.
public sealed class RawKernelDiagnosticTests
{
    /// <summary>가상 소멸·프레임 효과를 외부에서 기록하는 검사 전용 프로세스다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    private sealed class DiagnosticProcess : BaseProcess
    {
        /// <summary>같은 호출에서 대여한 외부 수명 관찰기다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
        private readonly Action? _destroy;

        /// <summary>슬롯 0은 이 관찰기에 도달하면 안 되는 프레임 관찰기다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
        private readonly Action? _frame;

        /// <summary>관찰기만 소유하고 게임 상태는 합성하지 않는다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
        /// <param name="destroy">소멸 관찰기다.</param>
        /// <param name="frame">프레임 관찰기다.</param>
        public DiagnosticProcess(Action? destroy = null, Action? frame = null)
        {
            _destroy = destroy;
            _frame = frame;
        }

        /// <summary>실제 가상 소멸 효과가 실행된 위치만 기록한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
        public override void Dispose() => _destroy?.Invoke();

        /// <summary>실행 순회가 선택한 실제 슬롯만 기록한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
        public override void RunFrame() => _frame?.Invoke();
    }

    /// <summary>빈 슬롯은 단언 뒤 다시 읽고 새로 등록된 현재 객체만 소멸시킨다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    [Fact]
    public void RemoveResumesWithCurrentSlotAndEdition()
    {
        // 두 원문의 경로·행 번호를 독립 상수로 대조하는 반복문이다. 2026-11-07 F2d 커널 이식에서 추가됐다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            // 빈 정상 복귀·슬롯 보수·단언 예외를 구별하는 반복문이다. 2026-11-07 F2d 커널 이식에서 추가됐다.
            for (int mode = 0; mode < 3; ++mode)
            {
                var kernel = new Kernel(2u);
                int assertions = 0;
                int destroyed = 0;
                var devices = new KernelDiagnostics { Edition = edition };
                devices.Assertion = (expression, file, line) =>
                {
                    ++assertions;
                    Assert.Equal("processes[pid] != NULL", expression);
                    Assert.Equal(edition == OriginalEdition.Patch1078 ? "\\Ns\\O\\Kernel.cpp" : "\\ns\\o\\Kernel.cpp", file);
                    Assert.Equal(edition == OriginalEdition.Patch1078 ? 0x82U : 0x7fU, line);
                    kernel.BindDiagnostics(new KernelDiagnostics());
                    if (mode == 1)
                    {
                        Assert.Equal(1u, kernel.Add(new DiagnosticProcess(() =>
                        {
                            Assert.NotNull(kernel.Get(1u));
                            Assert.Equal(1, kernel.Size());
                            ++destroyed;
                        })));
                    }

                    if (mode == 2)
                    {
                        throw new InvalidOperationException("선택된 단언 실패");
                    }
                };
                kernel.BindDiagnostics(devices);
                Assert.Equal(mode == 2, Throws(() => kernel.Remove(1u)));
                Assert.Equal(1, assertions);
                Assert.Equal(mode == 1 ? 1 : 0, destroyed);
                Assert.Equal(0, kernel.Size());
                Assert.Null(kernel.Get(1u));
                kernel.BindDiagnostics(devices);
                Assert.True(Throws(() => kernel.Remove(3u)));
                Assert.Equal(1, assertions);
            }
        }
    }

    /// <summary>원문의 요약→상세→단언 순서와 첫 슬롯 0을 보존한다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    [Fact]
    public void ExhaustionUsesCurrentDevicesAndCapturedZero()
    {
        // 두 원문의 상한과 별도로 축소 물리 입력의 실패 분기를 대조하는 반복문이다. 2026-11-07 F2d 커널 이식에서 추가됐다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            // 진단 중 빈 슬롯이 생겨도 0을 다른 번호로 재선택하면 안 되는 경우를 구별하는 반복문이다. 2026-11-07 F2d 커널 이식에서 추가됐다.
            foreach (bool freeDuringAssertion in new[] { false, true })
            {
                int destroyed = 0;
                var kernel = new Kernel(1u);
                Assert.Equal(1u, kernel.Add(new DiagnosticProcess(() => ++destroyed)));
                var devices = new KernelDiagnostics { Edition = edition };
                string calls = string.Empty;
                string detail = string.Empty;
                devices.Summary = () =>
                {
                    calls += 'S';
                    devices.Detail = text =>
                    {
                        calls += 'D';
                        detail = text;
                        devices.Assertion = (expression, file, line) =>
                        {
                            calls += 'A';
                            Assert.Equal("firstSlot != 0", expression);
                            Assert.Equal(edition == OriginalEdition.Patch1078 ? "\\Ns\\O\\Kernel.cpp" : "\\ns\\o\\Kernel.cpp", file);
                            Assert.Equal(edition == OriginalEdition.Patch1078 ? 0x7aU : 0x77U, line);
                            if (freeDuringAssertion)
                            {
                                kernel.Remove(1u);
                            }
                        };
                        kernel.BindDiagnostics(devices);
                    };
                    kernel.BindDiagnostics(devices);
                };
                kernel.BindDiagnostics(devices);
                var added = new DiagnosticProcess(() => ++destroyed);
                BaseProcess pointer = added;
                Assert.Equal(0u, kernel.Add(added));
                Assert.Equal("SDA", calls);
                Assert.Equal("Out of processes. A process count trace has been dumped\nto procSumm.txt.  Please deliver this file with the bug report.\n", detail);
                Assert.Same(pointer, kernel.Get(0u));
                Assert.Equal(freeDuringAssertion ? 1 : 2, kernel.Size());
                Assert.Equal(freeDuringAssertion ? 1 : 0, destroyed);
                Assert.Null(kernel.Get(2u));
                kernel.DrainOwnedForShutdown();
                Assert.Equal(2, destroyed);
                Assert.Equal(0, kernel.Size());
                Assert.Null(kernel.Get(0u));
            }
        }
    }

    /// <summary>각 진단 실패는 그 앞의 효과만 남기고 새 포인터를 슬롯에 기록하지 않는다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    [Fact]
    public void ExhaustionFailurePreservesSelectedPartialEffects()
    {
        // 요약·상세·단언에서 각각 예외를 전달하는 반복문이다. 2026-11-07 F2d 커널 이식에서 추가됐다.
        for (int stop = 0; stop < 3; ++stop)
        {
            int destroyed = 0;
            var kernel = new Kernel(1u);
            kernel.Add(new DiagnosticProcess());
            string calls = string.Empty;
            var devices = new KernelDiagnostics();
            devices.Summary = () =>
            {
                calls += 'S';
                if (stop == 0)
                {
                    throw new InvalidOperationException("요약 실패");
                }
            };
            devices.Detail = _ =>
            {
                calls += 'D';
                if (stop == 1)
                {
                    throw new InvalidOperationException("상세 실패");
                }
            };
            devices.Assertion = (_, _, _) =>
            {
                calls += 'A';
                throw new InvalidOperationException("단언 실패");
            };
            kernel.BindDiagnostics(devices);
            Assert.True(Throws(() => kernel.Add(new DiagnosticProcess(() => ++destroyed))));
            Assert.Equal("SDA"[..(stop + 1)], calls);
            Assert.Equal(1, destroyed);
            Assert.Equal(1, kernel.Size());
            Assert.Null(kernel.Get(0u));
            kernel.Clear();
        }

        var unbound = new Kernel(1u);
        unbound.Add(new DiagnosticProcess());
        Assert.True(Throws(() => unbound.Add(new DiagnosticProcess())));
        Assert.Equal(1, unbound.Size());
        Assert.Null(unbound.Get(0u));
    }

    /// <summary>슬롯 0 덮어쓰기에는 원문에 없는 소멸을 추가하지 않는다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    [Fact]
    public void ZeroOverwriteSkipsNativePassAndDrainsSelectedOwner()
    {
        string destroyed = string.Empty;
        string frames = string.Empty;
        var kernel = new Kernel(1u);
        var devices = new KernelDiagnostics
        {
            Summary = () => { },
            Detail = _ => { },
            Assertion = (_, _, _) => { },
        };
        kernel.BindDiagnostics(devices);
        kernel.Add(new DiagnosticProcess(() => destroyed += 'N', () => frames += 'N'));
        var first = new DiagnosticProcess(() => destroyed += 'A', () => frames += 'A');
        BaseProcess old = first;
        Assert.Equal(0u, kernel.Add(first));
        var second = new DiagnosticProcess(() => destroyed += 'B', () => frames += 'B');
        BaseProcess current = second;
        Assert.Equal(0u, kernel.Add(second));
        Assert.Equal(string.Empty, destroyed);
        Assert.Same(current, kernel.Get(0u));
        Assert.Equal(2, kernel.Size());
        kernel.RunFrame();
        Assert.Equal("N", frames);
        kernel.Clear();
        Assert.Equal("N", destroyed);
        Assert.Equal(1, kernel.Size());
        Assert.Same(current, kernel.Get(0u));
        kernel.DrainOwnedForShutdown(process => ReferenceEquals(process, old));
        Assert.Equal("NA", destroyed);
        Assert.Same(current, kernel.Get(0u));
        Assert.Equal(1, kernel.Size());
        kernel.Remove(0u);
        Assert.Equal("NAB", destroyed);
        Assert.Null(kernel.Get(0u));
        Assert.Equal(0, kernel.Size());
    }

    /// <summary>가상 소멸 중 새 슬롯 0도 마지막 NULL 쓰기 뒤에는 실행 표에 남지 않는다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    [Fact]
    public void ZeroDestructorReplacementKeepsLastNullWrite()
    {
        int oldDestroyed = 0;
        int newDestroyed = 0;
        var kernel = new Kernel(1u);
        var devices = new KernelDiagnostics
        {
            Summary = () => { },
            Detail = _ => { },
            Assertion = (_, _, _) => { },
        };
        kernel.BindDiagnostics(devices);
        kernel.Add(new DiagnosticProcess());
        kernel.Add(new DiagnosticProcess(() =>
        {
            ++oldDestroyed;
            Assert.NotNull(kernel.Get(0u));
            Assert.Equal(0u, kernel.Add(new DiagnosticProcess(() => ++newDestroyed)));
        }));
        kernel.Remove(0u);
        Assert.Equal(1, oldDestroyed);
        Assert.Equal(0, newDestroyed);
        Assert.Null(kernel.Get(0u));
        Assert.Equal(1, kernel.Size());
        kernel.DrainOwnedForShutdown();
        Assert.Equal(1, newDestroyed);
        Assert.Equal(0, kernel.Size());
    }

    /// <summary>생성자·조회·거부 경계가 원문과 같다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundariesMatchNativeShape()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => new Kernel(0u));
        Assert.Throws<ArgumentOutOfRangeException>(() => new Kernel(KernelShape.ProcessCapacity + 1u));
        var kernel = new Kernel(2u);
        Assert.Equal(2u, kernel.Capacity());
        Assert.Equal((uint)KernelShape.CdProcessCapacity, new Kernel(KernelShape.CdProcessCapacity).Capacity());
        var process = new DiagnosticProcess();
        Assert.Throws<ArgumentNullException>(() => kernel.Add(null!));
        Assert.Throws<ArgumentNullException>(() => kernel.AddRef(null!));
        Assert.Equal(0u, kernel.Find(process));
        Assert.Throws<ArgumentException>(() => kernel.AddRef(process));
        Assert.Equal(1u, kernel.Add(process));
        Assert.Equal(1u, kernel.AddRef(process));
        Assert.Equal(1u, kernel.Add(process));
        Assert.Equal(1, kernel.Size());
        Assert.Throws<ArgumentOutOfRangeException>(() => kernel.Remove(3u));
    }

    /// <summary>예외를 삼키고 성공 여부만 돌려주는 정적 보조 함수다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="operation">시도할 동작이다.</param>
    private static bool Throws(Func<object?> operation)
    {
        try
        {
            operation();
        }
        catch (Exception)
        {
            return true;
        }

        return false;
    }

    /// <summary>예외를 삼키고 성공 여부만 돌려주는 정적 보조 함수다. 2026-11-07 F2d 커널 이식에서 추가됐다.</summary>
    /// <param name="operation">시도할 동작이다.</param>
    private static bool Throws(Action operation)
    {
        try
        {
            operation();
        }
        catch (Exception)
        {
            return true;
        }

        return false;
    }
}
