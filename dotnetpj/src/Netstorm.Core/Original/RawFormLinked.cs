using System.Buffers.Binary;

// 공통 Form의 현재 부모와 종속 체인 전체를 검사하는 모듈을 담는다.
namespace Netstorm.Core.Original;

/// <summary>부모의 현재 종속 목록 끝까지 연결을 확인한다. cpppj/src/o/RawFormLinked.cpp의 004ae080 부모에 대응한다.</summary>
public static class RawFormLinked
{
    /// <summary>알려진 ProcessForm 가상 getter의 부모 WORD를 읽는다. TYPE·상태 필터를 추가하지 않는다.</summary>
    public static int ReadProcessFormParent(RawSidPool pool, int form)
    {
        ReadOnlySpan<byte> raw = pool.Slot(form);
        uint expected = pool.Edition == OriginalEdition.Patch1078 ? 0x512098u : 0x505990u;
        if (BinaryPrimitives.ReadUInt32LittleEndian(raw) != expected)
            throw new InvalidOperationException("Form 진단의 미복원 부모 가상 getter");
        return BinaryPrimitives.ReadUInt16LittleEndian(raw[16..]);
    }

    /// <summary>명시한 물리와 논리 범위로 체인 전체를 검사한다. 자기 뒤의 범위 실패도 false이며 순환은 명시 오류다.</summary>
    public static bool CheckChain(ReadOnlySpan<byte> bytes, int stride, int capacity, int form, int parent)
    {
        if (stride < 8 || (uint)capacity > 65535u || capacity > bytes.Length / stride)
            throw new ArgumentOutOfRangeException(nameof(capacity), "Form 연결 검사의 실제 풀 물리 범위");
        if ((uint)parent >= bytes.Length / stride)
            throw new ArgumentOutOfRangeException(nameof(parent), "Form 연결 검사의 부모 물리 범위");
        int current = BinaryPrimitives.ReadUInt16LittleEndian(bytes[(parent * stride + 6)..]);
        bool found = false;
        var visited = new HashSet<int>();
        // 자기를 발견한 뒤에도 남은 모든 종속 번호의 범위와 끝을 검사한다.
        while (current != 0)
        {
            if (current >= capacity) return false;
            if (!visited.Add(current)) throw new InvalidOperationException("Form 연결 검사의 원문 순환 호스트 중단");
            if (current == form) found = true;
            current = BinaryPrimitives.ReadUInt16LittleEndian(bytes[(current * stride + 4)..]);
        }
        return found;
    }

    /// <summary>가상 부모를 한 번 조회한 뒤 현재 풀을 새로 빌린다. getter가 풀을 재확보한 경우에도 이전 물리를 읽지 않는다.</summary>
    public static bool Check(RawSidPool pool, int form, Func<int, int>? parent)
    {
        if (parent is null) throw new InvalidOperationException("Form 연결 검사의 현재 부모 가상 장치 미연결");
        int selected = parent(form);
        return CheckChain(pool.Bytes, RawSidPool.Stride, pool.Capacity, form, selected);
    }
}
