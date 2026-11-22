using System.Diagnostics.CodeAnalysis;
using Netstorm.Assets;

namespace Netstorm.Core.Original; // 같은 파일/키 상태와 장기 경로 고리의 전체 출처 선택이다.

/// <summary>41a340이 반환한 등록 파일명과 부분 파일 메타데이터다.</summary>
public sealed class BaseFileArchiveOpen
{
    /// <summary>콜백 반환 뒤 소유하는 같은 실제 TAFF 이름이다.</summary>
    public ReadOnlyMemory<byte> FileName { get; }
    /// <summary>부분 파일 시작/길이의 signed32 비트다.</summary>
    public int Offset { get; }
    /// <summary>등록한 원문 부분 파일 길이다.</summary>
    public int Length { get; }
    /// <summary>0이 아니면 성공 뒤 고정 키를 설치한다.</summary>
    public uint Encrypted { get; }
    /// <summary>원문 값형 반환처럼 파일명 BYTE를 소유한다.</summary>
    public BaseFileArchiveOpen(ReadOnlyMemory<byte> fileName, int offset, int length, uint encrypted) { FileName = fileName.ToArray(); Offset = offset; Length = length; Encrypted = encrypted; }
}

/// <summary>실제 root/metadata/예외·검색·명시 Unicode 매핑 경계다.</summary>
public sealed class BaseFileOpenPolicyHooks
{
    /// <summary>실제 사용할 때 현재 기본 root를 빌린다.</summary>
    public Func<ReadOnlyMemory<byte>>? PrimaryRoot { get; set; }
    /// <summary>보조 디스크 도달 때 현재 보조 root를 빌린다.</summary>
    public Func<ReadOnlyMemory<byte>>? SecondaryRoot { get; set; }
    /// <summary>내용을 읽지 않고 같은 이름의 기본/보조 등록 메타데이터를 조회한다.</summary>
    public Func<ReadOnlyMemory<byte>, bool, BaseFileArchiveOpen?>? Archive { get; set; }
    /// <summary>현재 이름을 진단하고 반드시 예외를 던지는 실제 장치다.</summary>
    public Action<ReadOnlyMemory<byte>>? UnableToOpen { get; set; }
    /// <summary>정상/예외 이탈까지 유지하는 현재 FileSpec 검색 수명이다.</summary>
    public FileSpecQueryHooks Search { get; set; } = new();
    /// <summary>명시 ACP 소비자의 OS 이름 변환이며 null만 디스크 실패다.</summary>
    public Func<ReadOnlyMemory<byte>, ReadOnlyMemory<byte>, ReadOnlyMemory<byte>?>? DiskPath { get; set; }
}

/// <summary>cpppj/src/o/RawBaseFileOpenPolicy.h/.cpp의 전체 열기 부모다.</summary>
public static class RawBaseFileOpenPolicy
{
    /// <summary>원문 NUL을 포함한 고정 archive 키다.</summary>
    private static readonly byte[] ArchiveKey = "mydoghasfleas\0"u8.ToArray();
    /// <summary>선택된 하위만 호출 위치에서 요구한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("BaseFile 열기 부모의 선택된 장치 미연결");
    /// <summary>raw 이름은 NUL 물리를 필수로, 값형 root/이름은 현재 첫 NUL까지만 읽는다.</summary>
    private static ReadOnlyMemory<byte> Text(ReadOnlyMemory<byte> value, bool required = false)
    {
        int end = value.Span.IndexOf((byte)0); if (end < 0 && required) throw new ArgumentOutOfRangeException(nameof(value), "BaseFile 열기 이름의 NUL 물리 누락"); return end < 0 ? value : value[..end];
    }
    /// <summary>원문 비복귀 예외 장치의 정상 복귀를 빈 성공으로 바꾸지 않는다.</summary>
    [DoesNotReturn]
    private static void Unable(ReadOnlyMemory<byte> name, BaseFileOpenPolicyHooks hooks) { Need(hooks.UnableToOpen)(name); throw new InvalidOperationException("BaseFile의 원문 비복귀 예외 장치가 복귀함"); }
    /// <summary>FileSpec→같은 Device 고리→실제 선택 순서/키/현재 suffix와 반납을 유지한다.</summary>
    public static void OpenBaseFileByPolicy(RawBaseFileState state, ReadOnlyMemory<byte> name, uint mode, uint create, RawFileName names, RawBaseFileIOHooks io, BaseFileOpenPolicyHooks hooks)
    {
        using var spec = new FileSpecQuery(Text(name, true), hooks.Search); bool device = !names.Device(spec.Text).IsEmpty;
        // 한 번 얻은 지역 메타데이터로 실제 archive FILE을 열고 성공 뒤에만 키를 설치한다.
        bool Archive(ReadOnlyMemory<byte> query, bool secondary)
        {
            var candidate = Need(hooks.Archive)(Text(query, true), secondary); if (candidate is null) return false;
            if (!RawBaseFileIO.OpenBaseFileSubfile(state, candidate.FileName, candidate.Offset, candidate.Length, io)) Unable(candidate.FileName, hooks);
            if (candidate.Encrypted != 0) RawBaseFileIO.SetBaseFileKey(state, ArchiveKey, io); return true;
        }
        // 직접 장치만 Format을 생략하며 Unicode 매핑을 선택해도 선행 고리 효과는 남긴다.
        bool Disk(ReadOnlyMemory<byte> root, ReadOnlyMemory<byte> query, uint selectedMode, bool direct = false)
        {
            byte[] raw = direct ? query.ToArray() : names.Format(new FileNamePart[] { new(-2, root), new(-3, query) }).ToArray();
            if (hooks.DiskPath is null) return RawBaseFileIO.OpenBaseFileDisk(state, raw, selectedMode, create, io);
            var mapped = hooks.DiskPath(root, query); return mapped.HasValue && RawBaseFileIO.OpenBaseFileDisk(state, mapped.Value, selectedMode, create, io);
        }
        if (!device)
        {
            if (Disk(Text(Need(hooks.PrimaryRoot)()), Text(name, true), mode)) return;
            if (Archive(name, false)) return;
            if (Disk(Text(Need(hooks.SecondaryRoot)()), Text(name, true), mode)) return;
            if (Archive(name, true)) return; Unable(Text(name, true), hooks);
        }
        if (Disk(ReadOnlyMemory<byte>.Empty, Text(name, true), mode, true)) return;
        if (mode != 0) Unable(Text(name, true), hooks);
        var root = Text(Need(hooks.PrimaryRoot)()); var current = Text(name, true); int common = 0;
        // 대소문자/구분자/UTF-8 경계를 정규화하지 않고 첫 불일치 BYTE까지 이동한다.
        while (common < current.Length && common < root.Length && current.Span[common] == root.Span[common]) common++;
        var suffix = name[common..]; if (Archive(suffix, false)) return;
        if (Disk(Text(Need(hooks.SecondaryRoot)()), Text(suffix, true), 0)) return;
        if (Archive(suffix, true)) return; Unable(Text(name, true), hooks);
    }
}
