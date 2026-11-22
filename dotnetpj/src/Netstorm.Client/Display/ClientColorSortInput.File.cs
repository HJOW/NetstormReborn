using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Display; // 실제 사용자 캐시 FILE과 같은 장기 색표 수명을 연결한다.

/// <summary>Source ClientColorMapFile.cpp의 실제 reader/writer 조립과 같은9표 입출력이다.</summary>
public sealed partial class ClientColorSortInput
{
    /// <summary>동기 최초 캐시 부모에 두 실제 FILE/키 owner와 현재 등록/경로 환경을 공급한다.</summary>
    public void InitializeCache(GameFileSystem files, string path, uint currentRed, Action initializeOwners, Func<IRawBaseFileDevice> devices)
    {
        _ = Primary; if (MapsReady || initializeOwners is null) throw new InvalidOperationException("색표 캐시 최초 초기화 계약");
        using IRawBaseFileDevice reader = devices(); using IRawBaseFileDevice writer = devices(); byte[] readObject = new byte[24], writeObject = new byte[24], scratch = new byte[4096];
        ClientAllocationBytes.ReadCurrent(readObject); ClientAllocationBytes.ReadCurrent(writeObject); byte[] name = Encoding.UTF8.GetBytes(path + '\0');
        var hooks = new ColorMapCacheHooks
        {
            Path = () => name, Owners = initializeOwners,
            OpenRead = (_, _, mode, create) => RawBaseFileEnvironment.OpenByPolicy(files, reader.State, name, mode, create, reader.IO),
            OpenWrite = (_, _, mode, create) => RawBaseFileEnvironment.OpenByPolicy(files, writer.State, name, mode, create, writer.IO),
            Read = (_, table, offset) => { RawBaseFileIO.ReadBaseFile(reader.State, table, unchecked((int)offset), reader.IO, scratch); return 0; },
            Write = (_, table, offset) => { RawBaseFileIO.WriteBaseFile(writer.State, table, unchecked((int)offset), writer.IO); return 0; },
            Close = file => { IRawBaseFileDevice selected = file.Equals(readObject.AsMemory()) ? reader : writer; RawBaseFileIO.DestructBaseFile(selected.State, selected.IO); }
        };
        InitializeCache(hooks, readObject, writeObject, currentRed);
    }
}
