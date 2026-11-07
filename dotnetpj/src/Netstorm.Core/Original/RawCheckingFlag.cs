// 원문 005e4794의 전체 현재 DWORD를 소비 위치마다 읽는 비소유 진단 뷰다 (cpppj o/RawCheckingFlag.h, 2026-11-07 F2c-12 이식).
using System;

namespace Netstorm.Core.Original;

/// <summary>
/// Client owner와 독립 raw 진단 입력을 같은 비영 관문으로 읽는 비소유 진단 뷰다(cpppj <c>o/RawCheckingFlag.h</c>의 <c>RawCheckingFlag</c>).
/// 외부 저장소를 대리자로 빌리거나 독립 자체값을 보관하며, 각 진단 소비 위치에서 <see cref="Read"/>로 현재 비트를 다시 읽는다 (2026-11-07 F2c-12 이식).
/// </summary>
public sealed class RawCheckingFlag
{
    /// <summary>실제 ReadInt의 같은 장기 signed owner 읽기 대리자다 (2026-11-07 F2c-12 이식).</summary>
    private Func<int>? _signed;

    /// <summary>원문 독립 DWORD의 비소유 읽기 대리자다 (2026-11-07 F2c-12 이식).</summary>
    private Func<uint>? _word;

    /// <summary>기존 bool 참조 입력의 비소유 읽기 대리자다 (2026-11-07 F2c-12 이식).</summary>
    private Func<bool>? _boolean;

    /// <summary>기본·리터럴로 만든 독립 raw 입력에만 쓰는 자체값이다 (2026-11-07 F2c-12 이식).</summary>
    private uint _local;

    /// <summary>독립 raw 입력의 최초 checking 0을 만든다 (2026-11-07 F2c-12 이식).</summary>
    public RawCheckingFlag()
    {
    }

    /// <summary>불리언 초기값을 자체값으로 보관하는 진단 뷰를 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">초기 불리언 값.</param>
    public RawCheckingFlag(bool value)
    {
        _local = value ? 1u : 0u;
    }

    /// <summary>부호 없는 32비트 초기값을 자체값으로 보관하는 진단 뷰를 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">초기 DWORD 값.</param>
    public RawCheckingFlag(uint value)
    {
        _local = value;
    }

    /// <summary>부호 있는 32비트 초기값의 비트를 그대로 자체값으로 보관하는 진단 뷰를 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">초기 signed DWORD 값.</param>
    public RawCheckingFlag(int value)
    {
        _local = unchecked((uint)value);
    }

    /// <summary>원래 owner 대리자 또는 독립 자체값을 복사해 같은 장기 owner를 가리키는 사본을 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="other">복사할 원본 뷰.</param>
    public RawCheckingFlag(RawCheckingFlag other)
    {
        ArgumentNullException.ThrowIfNull(other);
        _signed = other._signed;
        _word = other._word;
        _boolean = other._boolean;
        _local = other._local;
    }

    /// <summary>독립 입력의 불리언 대입 의미를 유지하며 외부 owner에는 쓰지 않는다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">설정할 불리언 값.</param>
    public void Set(bool value)
    {
        Clear();
        _local = value ? 1u : 0u;
    }

    /// <summary>독립 입력의 DWORD 값을 비트를 좁히지 않고 설정한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">설정할 DWORD 값.</param>
    public void Set(uint value)
    {
        Clear();
        _local = value;
    }

    /// <summary>signed 독립 입력의 상위 비트도 유지해 설정한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="value">설정할 signed DWORD 값.</param>
    public void Set(int value)
    {
        Set(unchecked((uint)value));
    }

    /// <summary>같은 ReadInt·DebugSwitch signed owner에 명시 연결한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="getter">현재 signed DWORD를 읽는 대리자.</param>
    public void Bind(Func<int> getter)
    {
        ArgumentNullException.ThrowIfNull(getter);
        Clear();
        _signed = getter;
    }

    /// <summary>원문 DWORD 독립 관측도 동일 저장소에서 읽도록 명시 연결한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="getter">현재 DWORD를 읽는 대리자.</param>
    public void Bind(Func<uint> getter)
    {
        ArgumentNullException.ThrowIfNull(getter);
        Clear();
        _word = getter;
    }

    /// <summary>독립 bool 소유자의 기존 참조 계약을 보존하도록 명시 연결한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="getter">현재 bool을 읽는 대리자.</param>
    public void Bind(Func<bool> getter)
    {
        ArgumentNullException.ThrowIfNull(getter);
        Clear();
        _boolean = getter;
    }

    /// <summary>외부 효과 뒤 현재 비트를 다시 읽는다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>현재 32비트 플래그 값.</returns>
    public uint Read()
    {
        if (_signed is not null)
        {
            return unchecked((uint)_signed());
        }
        if (_word is not null)
        {
            return _word();
        }
        if (_boolean is not null)
        {
            return _boolean() ? 1u : 0u;
        }
        return _local;
    }

    /// <summary>관문만 비영으로 판정하며 원래 저장소를 0·1로 고치지 않는다 (2026-11-07 F2c-12 이식).</summary>
    public bool IsEnabled => Read() != 0u;

    /// <summary>C++ <c>operator bool()</c>과 같이 현재 값이 0이 아니면 참으로 판정한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="flag">판정할 플래그 뷰.</param>
    public static implicit operator bool(RawCheckingFlag? flag) => flag is not null && flag.Read() != 0u;

    /// <summary>명시 재연결 때 기존 비소유 대리자를 모두 버린다 (2026-11-07 F2c-12 이식).</summary>
    private void Clear()
    {
        _signed = null;
        _word = null;
        _boolean = null;
    }
}
