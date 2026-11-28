using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

// 10.78의 현재 FILE을 열어 같은 저장 팔레트에 직접 기록하고 적용 후단 뒤에 반납한다.
namespace Netstorm.Client.Display;

/// <summary>원문4a4850의 열기·적용 후단·길이 진단이다. 실제 도달한 장치만 호출 위치에서 요구한다.</summary>
public sealed class PaletteFileReadHooks
{
    /// <summary>같은 현재 File을 mode0/create1로 연다. 정상 복귀 전 실패는 본체의 File 정리에 들어가지 않는다.</summary>
    public Action<RawBaseFileState, ReadOnlyMemory<byte>, uint, uint>? Open { get; init; }
    /// <summary>현재 저장 팔레트의 clear/현재·논리 팔레트/색표 후단이다. 복귀할 때까지 파일을 열어 둔다.</summary>
    public Action? Apply { get; init; }
    /// <summary>잘못된 길이에 Screen.cpp940을 보고한다. 복귀하면 기존 저장 팔레트로 후단을 계속한다.</summary>
    public Action<uint>? Assertion { get; init; }
}

/// <summary>cpppj PaletteFileRead.cpp의 원문 FILE/부분 읽기 부모다. 완성 GamePalette 값 decoder와 구별한다.</summary>
public static class PaletteFileRead
{
    /// <summary>같은 저장 팔레트의 BGRX256칸 물리 크기다.</summary>
    public const int SavedBytes = 1024;
    /// <summary>현재 실제 파일명은 NUL을 포함한 UTF-8이며 경로 변환은 기존 FILE 환경에 맡긴다.</summary>
    private static readonly Encoding FileNames = new UTF8Encoding(false, true);
    /// <summary>미선택 callback은 앞서 요구하지 않고 원문 도달 위치에서 실패한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("팔레트 파일의 선택된 실제 장치 미연결");

    /// <summary>열기→길이/직접 쓰기→같은 Screen 후단→File 정리를 실행하며 실패 접두를 되돌리지 않는다.</summary>
    public static void Read(RawBaseFileState state, ReadOnlyMemory<byte> name, Memory<byte> saved,
        RawBaseFileIOHooks io, PaletteFileReadHooks hooks, Memory<byte> scratch)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(io); ArgumentNullException.ThrowIfNull(hooks);
        if (saved.Length != SavedBytes) throw new ArgumentException("팔레트의 실제 저장 물리는1024BYTE여야 합니다", nameof(saved));
        state.Key = 0; state.KeyLength = 0;
        Need(hooks.Open)(state, name, 0, 1);
        try
        {
            if (RawBaseFileIO.BaseFileLength(state, io) == 0x308)
            {
                RawBaseFileIO.SeekBaseFile(state, 8, 0, io);
                // 각1BYTE 읽기가 RGB를 현재 BGRX칸에 쓰며 예약BYTE와 짧은 읽기의 이전값을 유지한다.
                for (int index = 0; index < 256; index++)
                {
                    RawBaseFileIO.ReadBaseFile(state, saved.Slice(index * 4 + 2, 1), -1, io, scratch);
                    RawBaseFileIO.ReadBaseFile(state, saved.Slice(index * 4 + 1, 1), -1, io, scratch);
                    RawBaseFileIO.ReadBaseFile(state, saved.Slice(index * 4, 1), -1, io, scratch);
                }
            }
            else if (RawBaseFileIO.BaseFileLength(state, io) == 0x400)
                RawBaseFileIO.ReadBaseFile(state, saved, -1, io, scratch);
            else Need(hooks.Assertion)(0x940);
            Need(hooks.Apply)();
        }
        catch
        {
            RawBaseFileIO.DestructBaseFile(state, io);
            throw;
        }
        // 정상 소멸 자체가 실패했을 때 이 본체에서 두 번 소멸하지 않는다.
        RawBaseFileIO.DestructBaseFile(state, io);
    }

    /// <summary>현재 VFS metadata/backing FILE과 실제 키 소유자를 적용 후단까지 유지한다. 준비/실패 성공을 합성하지 않는다.</summary>
    public static void ReadNative(GameFileSystem files, string name, Memory<byte> saved,
        Action apply, Func<IRawBaseFileDevice> devices, Action<uint>? assertion = null)
    {
        ArgumentNullException.ThrowIfNull(files); ArgumentNullException.ThrowIfNull(name);
        ArgumentNullException.ThrowIfNull(apply); ArgumentNullException.ThrowIfNull(devices);
        using IRawBaseFileDevice reader = devices();
        byte[] path = FileNames.GetBytes(name + '\0'), scratch = new byte[4096];
        Read(reader.State, path, saved, reader.IO, new()
        {
            Open = (state, current, mode, create) => RawBaseFileEnvironment.OpenByPolicy(files, state, current, mode, create, reader.IO),
            Apply = apply,
            Assertion = assertion ?? (line => throw new InvalidOperationException("팔레트 파일 부모의 원문 진단 " + line)),
        }, scratch);
        // using은 미완성 Open의 현대 OS 자원도 회수한다. 미작성 암호 scratch의0은 원문 미정 stack의 증명이 아니다.
    }
}
