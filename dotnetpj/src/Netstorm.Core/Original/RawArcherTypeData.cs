// 실제 TYPE callback의 orientations float 절삭과 클러스터 수를 원문 정의에서 읽는다.
namespace Netstorm.Core.Original;

/// <summary>RawArcherTypeData.cpp의 실제 방향/물리 프레임 자료 공급이다.</summary>
public static class RawArcherTypeData
{
    /// <summary>누락 방향은0, 명시 orientations는 숫자 AST의 float폭으로 읽고0쪽으로 절삭한다.</summary>
    public static ArcherTypeData Read(RiftTypeDefinition definition)
    {
        if (definition.Property("orientations") is not null && definition.Number("orientations") is null) throw new ArgumentException("아처 방향수 숫자 AST 원문 계약 오류"); double? number = definition.Number("orientations"); float value = number is null ? 0 : (float)number.Value; if (!float.IsFinite(value) || (double)value < int.MinValue || (double)value > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(definition), "아처 방향수 CRT 정상 정수 범위 오류"); return new((int)value, definition.Clusters.Count);
    }
}
