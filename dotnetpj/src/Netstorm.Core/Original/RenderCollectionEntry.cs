namespace Netstorm.Core.Original; // 원본 월드·표시 자료의 같은 배치를 보존하는 영역이다.

/// <summary>원본 공통 16바이트 그리기 항목. 수집·정렬·출력이 같은 값을 전달한다.
/// 근거: cpppj/src/o/RawRenderCollection.h. 수집기 본체는 R5에서 연결한다.</summary>
/// <param name="X">객체 또는 부모의 월드 가로 좌표.</param>
/// <param name="Y">객체 또는 부모의 월드 세로 좌표.</param>
/// <param name="Token">원본 포인터 대신 쓰는 DWORD 식별자.</param>
/// <param name="Depth">zOrder의 하위 WORD.</param>
/// <param name="Kind">출력 종류(일반 객체는 0).</param>
/// <param name="Tail">그림자 등 종류별 부가 BYTE.</param>
public readonly record struct RenderCollectionEntry(float X, float Y, uint Token, short Depth, byte Kind, byte Tail);
