namespace Netstorm.Assets; // 원문 BYTE 경로와 실제 검색 핸들 수명을 보존한다.

/// <summary>FindFirst의 실제 호스트 핸들과 첫 원문 이름이다.</summary>
/// <param name="Handle">실패도 보존하는 호스트 폭 핸들이다.</param><param name="Name">첫 NUL 앞까지 사용할 원시 BYTE 이름이다.</param>
public readonly record struct FileSpecFindResult(nuint Handle, ReadOnlyMemory<byte> Name);

/// <summary>원문 검색 장치다. Find/Close가 모두 없으면 실제 Windows 장치를 사용한다.</summary>
public sealed class FileSpecQueryHooks
{
    /// <summary>정규화 전 원래 패턴의 첫 검색이다.</summary>
    public Func<ReadOnlyMemory<byte>, FileSpecFindResult>? Find { get; set; }
    /// <summary>비NULL·실패 핸들도 한 번 반납하며 예외를 던지지 않는 장치다.</summary>
    public Action<nuint>? Close { get; set; }
    /// <summary>null만 검색 종료이며 빈 이름은 정상 반환이다.</summary>
    public Func<nuint, ReadOnlyMemory<byte>?>? Next { get; set; }
}

/// <summary>cpppj/src/o/FileSpecQuery.h/.cpp의 생성/재대입/다음 검색/반납 전체다.</summary>
public sealed partial class FileSpecQuery : IDisposable
{
    /// <summary>생성 때 값으로 복사한 같은 장치들이다.</summary>
    private readonly FileSpecQueryHooks _hooks;
    /// <summary>현재 검색을 연 실제 핸들이다.</summary>
    private nuint _handle;
    /// <summary>현재 핸들과 함께 보존한 반납 장치다.</summary>
    private Action<nuint>? _close;
    /// <summary>현재 원시 BYTE 문자열의 소유 배열이다.</summary>
    private byte[] _value = [];
    /// <summary>IDisposable의 중복 반납만 막는 호스트 표식이다.</summary>
    private bool _disposed;
    /// <summary>원문+104의 유효 표식이며 일반 이름은 파일 조회 없이 true다.</summary>
    public bool Valid { get; private set; } = true;
    /// <summary>새 고리나 검색 없이 현재 문자열을 빌린다.</summary>
    public ReadOnlyMemory<byte> Text => _value;
    /// <summary>정상/예외 모두 이미 얻은 검색 핸들의 수명을 관리한다.</summary>
    public FileSpecQuery(ReadOnlyMemory<byte> name, FileSpecQueryHooks? hooks = null)
    {
        _hooks = new() { Find = hooks?.Find, Close = hooks?.Close, Next = hooks?.Next };
        try { Assign(name); } catch { Dispose(); throw; }
    }
    /// <summary>Source string_view처럼 첫 NUL 앞의 현재 입력만 빌린다.</summary>
    private static ReadOnlyMemory<byte> CString(ReadOnlyMemory<byte> value) { int end = value.Span.IndexOf((byte)0); return end < 0 ? value : value[..end]; }
    /// <summary>새 입력 정규화 뒤 이전 핸들을 닫고 원래 패턴으로 다시 검색한다.</summary>
    public void Assign(ReadOnlyMemory<byte> name)
    {
        ObjectDisposedException.ThrowIf(_disposed, this); byte[] input = CString(name).ToArray();
        bool prefix = input.AsSpan().IndexOf((byte)'\\') < 0 && input.AsSpan().IndexOf((byte)':') < 0;
        _value = new byte[input.Length + (prefix ? 2 : 0)]; if (prefix) { _value[0] = (byte)'.'; _value[1] = (byte)'\\'; } input.CopyTo(_value, prefix ? 2 : 0);
        // 접두 판단은 slash 변환보다 먼저이며 같은 입력 BYTE의 순서를 유지한다.
        for (int index = 0; index < _value.Length; index++) if (_value[index] == (byte)'/') _value[index] = (byte)'\\';
        Valid = true;
        if (_handle != 0) { _close!(_handle); _handle = 0; }
        if (input.AsSpan().IndexOfAny((byte)'*', (byte)'?') < 0) return;
        if ((_hooks.Find is null) != (_hooks.Close is null)) throw new InvalidOperationException("FileSpec 검색/반납 장치는 함께 필요");
        _close = _hooks.Close ?? WindowsClose; var found = _hooks.Find is not null ? _hooks.Find(input) : WindowsFind(input); _handle = found.Handle;
        Valid = found.Handle != nuint.MaxValue; if (!Valid) { _value = []; return; }
        int slash = input.AsSpan().LastIndexOf((byte)'\\'); _value = Concat(input.AsMemory(0, slash + 1), CString(found.Name));
    }
    /// <summary>두 원문 뷰를 정규화 없이 소유 문자열로 연결한다.</summary>
    private static byte[] Concat(ReadOnlyMemory<byte> prefix, ReadOnlyMemory<byte> name)
    {
        byte[] value = new byte[checked(prefix.Length + name.Length)]; prefix.CopyTo(value); name.CopyTo(value.AsMemory(prefix.Length)); return value;
    }
    /// <summary>종료에서 마지막 문자열은 남기며 명시 Find의 Next는 선택 위치에서만 요구한다.</summary>
    public bool Next()
    {
        ObjectDisposedException.ThrowIf(_disposed, this); if (_handle == 0) { Valid = false; return false; } if (!Valid) return false;
        if (_hooks.Find is not null && _hooks.Next is null) throw new InvalidOperationException("FileSpec의 명시 다음검색 장치 없음");
        ReadOnlyMemory<byte>? found = _hooks.Next is not null ? _hooks.Next(_handle) : WindowsNext(_handle); Valid = found.HasValue; if (!found.HasValue) return false;
        int slash = _value.AsSpan().LastIndexOf((byte)'\\'); _value = Concat(_value.AsMemory(0, slash + 1), CString(found.Value)); return true;
    }
    /// <summary>현재 실제 비NULL 핸들을 한 번 닫으며 실패 핸들도 반납한다.</summary>
    public void Dispose()
    {
        if (_disposed) return; if (_handle != 0) { _close!(_handle); _handle = 0; } _disposed = true;
    }
}
