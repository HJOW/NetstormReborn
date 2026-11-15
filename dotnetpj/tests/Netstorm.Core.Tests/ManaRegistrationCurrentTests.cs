using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 CPU 공급원 등록의 signed 개수·용량·진단 뒤 현재 owner를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj ManaRegistrationTests의1084행과 선택된 장치/부분 효과를 옮긴다.</summary>
public sealed class ManaRegistrationCurrentTests
{
    /// <summary>원본 전체 DWORD와 두 현재 배열을 분리해 유지한다.</summary>
    private sealed class Scene
    {
        /// <summary>최초 실제 목록의 stale 꼬리다.</summary>
        public readonly uint[] A = Enumerable.Range(0, 16).Select(index => 0x11220000u + (uint)index).ToArray();
        /// <summary>교체 실제 목록의 stale 꼬리다.</summary>
        public readonly uint[] B = Enumerable.Range(0, 16).Select(index => 0x33440000u + (uint)index).ToArray();
        /// <summary>현재 배열 선택 owner다.</summary>
        public uint Pointer;
        /// <summary>현재 전체 사용 개수다.</summary>
        public uint Used;
        /// <summary>현재 전체 용량이다.</summary>
        public uint Capacity;
        /// <summary>현재 생산 알림 카운터다.</summary>
        public uint Dirty;
        /// <summary>현재 전체 checking이다.</summary>
        public uint Checking;
        /// <summary>검색/저장하는 전체 DWORD 토큰이다.</summary>
        public uint Value;
        /// <summary>진단 callback 변경 입력이다.</summary>
        public uint Action;
        /// <summary>같은 현재 목록 뷰다.</summary>
        public readonly RawPostPopList List = new();
        /// <summary>현재 진단 owner다.</summary>
        public readonly ManaRegistrationDiagnostics Diagnostics;
        /// <summary>실제 선택한 진단 순서다.</summary>
        public string Events = "";

        /// <summary>CPU에 공급한 입력만 옮기며 최종 등록 결과는 만들지 않는다.</summary>
        public Scene(string[] row)
        {
            Value = X86Fixture.UInt(row[2]); Used = X86Fixture.UInt(row[3]); Capacity = X86Fixture.UInt(row[4]); Dirty = X86Fixture.UInt(row[6]); Checking = X86Fixture.UInt(row[7]); Action = X86Fixture.UInt(row[8]); uint pattern = X86Fixture.UInt(row[5]);
            if (Used != 0 && (pattern & 1) != 0) A[0] = Value; if (Used != 0 && (pattern & 2) != 0) A[Used - 1] = Value;
            List.BindCurrent(new() { Storage = () => Pointer != 0 ? B : A, Capacity = () => Capacity, ReadCount = () => Used, WriteCount = count => Used = count });
            Diagnostics = new() { Checking = new(() => Checking), Assertion = (expression, file, line) =>
            {
                Events += $"D,{line}:{expression}:{file};"; if ((Action & 1) != 0) Checking = 0; if ((Action & 2) != 0) { Pointer = 1; Used = 0; Capacity = 2; } if ((Action & 4) != 0) Dirty = uint.MaxValue;
                if ((Action & 8) != 0)
                {
                    // 진단에서 실제 기존 배열만 바꾸고 저장된 중복 판단은 재계산하지 않는다.
                    for (int index = 0; index < A.Length; index++) if (A[index] == Value) A[index] = 0xdead0000u + (uint)index;
                }
            } };
        }
        /// <summary>두 배열의 모든 현재 DWORD를 원문 바이트 순서로 직렬화한다.</summary>
        private static string Words(uint[] values) => Convert.ToHexStringLower(values.SelectMany(BitConverter.GetBytes).ToArray());
        /// <summary>출력 모델 없이 현재 메모리/전역만 읽는다.</summary>
        public string[] Result() => [Pointer.ToString(), Used.ToString(), Capacity.ToString(), Dirty.ToString(), Checking.ToString(), Words(A), Words(B), Events == "" ? "-" : Events];
    }

    /// <summary>두 원본의 실제 CPU1084행을 전체 목록·현재 전역·진단 순서로 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    public void NativeSignedCapacityDuplicateResumeAndCurrentCounter(string editionName, OriginalEdition edition)
    {
        int count = 0;
        // 각 원문 행을 독립 입력으로 재생하며 기대값은 fixture에서만 읽는다.
        foreach (string[] row in X86Fixture.Read("mana-registration-hardware").Where(row => row[0] == editionName))
        {
            var s = new Scene(row); RawManaRegistration.Register(edition, s.List, ref s.Dirty, s.Value, s.Diagnostics); Assert.Equal(row.Skip(9), s.Result()); count++;
        }
        Assert.Equal(542, count);
    }

    /// <summary>최초 양수 count 뒤 첫 중복 검사는 다음 현재 count 읽기보다 먼저다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void FirstSearchPrecedesNextCurrentCountRead(OriginalEdition edition)
    {
        uint[] words = [50]; uint used = 1, reads = 0, dirty = 0; var list = new RawPostPopList();
        list.BindCurrent(new() { Storage = () => words, Capacity = () => 1, ReadCount = () => { uint value = used; if (++reads == 1) used = 0; return value; }, WriteCount = value => used = value });
        RawManaRegistration.Register(edition, list, ref dirty, 50, new()); Assert.Equal(0u, used); Assert.Equal(1u, reads); Assert.Equal(1u, dirty); Assert.Equal(50u, words[0]);
    }

    /// <summary>선택된 진단 누락/중단의 부분 효과를 보존하며 signed 음수 용량은 배열을 선택하지 않는다.</summary>
    [Fact]
    public void SelectedDevicesAndPartialEffects()
    {
        string[] row = X86Fixture.Read("mana-registration-hardware")[0].ToArray(); row[2] = "50"; row[3] = "1"; row[4] = "4"; row[5] = "1"; row[6] = "123"; row[7] = "2147483648";
        var missing = new Scene(row); missing.Diagnostics.Assertion = null;
        Assert.Throws<InvalidOperationException>(() => RawManaRegistration.Register(OriginalEdition.Patch1078, missing.List, ref missing.Dirty, missing.Value, missing.Diagnostics)); Assert.Equal(123u, missing.Dirty); Assert.Equal(1u, missing.Used);
        var stopped = new Scene(row); stopped.Diagnostics.Assertion = (_, _, _) => { stopped.Pointer = 1; stopped.Used = 0; stopped.Dirty = 42; throw new InvalidOperationException("선택 진단 중단"); };
        Assert.Throws<InvalidOperationException>(() => RawManaRegistration.Register(OriginalEdition.Patch1078, stopped.List, ref stopped.Dirty, stopped.Value, stopped.Diagnostics)); Assert.Equal(1u, stopped.Pointer); Assert.Equal(0u, stopped.Used); Assert.Equal(42u, stopped.Dirty); Assert.Equal(50u, stopped.A[0]);
        uint dirty = uint.MaxValue; var noArray = new RawPostPopList();
        noArray.BindCurrent(new() { Storage = () => throw new InvalidOperationException("미선택 배열"), Capacity = () => 0x80000000, ReadCount = () => 0, WriteCount = _ => throw new InvalidOperationException("미선택 개수 쓰기") });
        RawManaRegistration.Register(OriginalEdition.Patch1078, noArray, ref dirty, uint.MaxValue, new()); Assert.Equal(0u, dirty);
    }
}
