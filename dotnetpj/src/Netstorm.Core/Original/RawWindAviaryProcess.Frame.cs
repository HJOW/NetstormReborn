using System.Buffers.Binary;

// Wind의 임시 표시/실제 비행 시작과 원문 효과 뒤 현재 필드 재조회를 구별한다.
namespace Netstorm.Core.Original;

/// <summary>RawWindAviaryProcess.cpp의 실제1~13 상태기계다.</summary>
public sealed partial class RawWindAviaryProcess
{
    /// <summary>현재생성 뒤 임시DWORD/frame13/처음부모의 현재owner를 원문순서로 쓴다.</summary>
    private ushort Create(WindAviaryProcessFields fields, ushort firstParent)
    {
        ushort sid = Need(_hooks.Create)(_globals.FlyerType.Value); Flyer(fields, sid); Put(Pool.AllocatedBytes(sid), 36, 13); Need(_hooks.SetOwner)(sid, Pool.Slot(firstParent)[34]); return sid;
    }
    /// <summary>x87의0방향 int절삭과 범위밖 indefiniteDWORD를 명시한다.</summary>
    private static int Truncate(double value) => !double.IsFinite(value) || value < int.MinValue || value >= 2147483648.0 ? int.MinValue : (int)Math.Truncate(value);
    /// <summary>처음부모/raw와 선택효과 뒤현재state/parent/flyer/clock을 구별한다.</summary>
    public void Frame(WindAviaryProcessFields fields)
    {
        ushort parent = Parent(fields); ReadOnlySpan<byte> raw = Pool.Slot(parent); if ((raw[40] & 0x20) != 0 || _globals.Editor() != 0) return;
        if (Read(fields.Bytes, 24) == 1 && raw[10] != _globals.AviaryType.Value) State(fields, 11); uint stage = Read(fields.Bytes, 24);
        switch (stage)
        {
            case 1:
                fields.Bytes[8] |= 2; Time(fields, 16, _globals.Now()); Time(fields, 16, _globals.Now() + 1.0); State(fields, 2); Time(fields, 32, _globals.Now()); fields.Bytes[8] |= 0x30; Time(fields, 40, _globals.Now()); Put(fields.Bytes, 48, 0); return;
            case 2: case 7:
                float elapsed = (float)(_globals.Now() - Time(fields, 40)); double delay = 0; uint current = stage;
                if (Time(fields, 32) != 0 && Time(fields, 32) <= _globals.Now())
                {
                    int next = unchecked((int)(Read(raw, 36) + 1u)) % 6; if (_globals.Checking.IsEnabled) Need(_hooks.CheckFrame)(parent, next, 0x884); Need(_hooks.SetFrame)(parent, next);
                    current = Read(fields.Bytes, 24); delay = .3f;
                    if (current == 7 || elapsed < 2.0f)
                    {
                        double delta = current == 7 ? (double)elapsed : (double)elapsed - 2.0, scaled = delta * delta * (double).05f;
                        delay = scaled > 0 && scaled > .3f ? (double).3f : scaled <= 0 ? 0 : scaled;
                    }
                    else delay = 0;
                    Time(fields, 32, _globals.Now() + delay);
                }
                if (current == 7) { if (!(delay < .3f)) State(fields, 5); return; }
                if (Read(fields.Bytes, 48) != 0 && elapsed < 6.0f)
                {
                    int converted = Truncate(((double)elapsed - 4.0) * -4.5), frame = Math.Clamp(unchecked(13 - converted), 13, 21); ushort sid = Flyer(fields);
                    if (_globals.Checking.IsEnabled) Need(_hooks.CheckFrame)(sid, frame, 0x8a6); Need(_hooks.SetFrame)(sid, frame);
                }
                if ((Read(fields.Bytes, 48) != 0 || !_globals.Boss()) && elapsed > 6.0f)
                {
                    if (_globals.Server()) { ushort sid = Flyer(fields); Need(_hooks.CreateFlyerProcess)(sid, Parent(fields)); Need(_hooks.BindFlyer)(Flyer(fields), Parent(fields)); Flyer(fields, 0); }
                    fields.Bytes[8] |= 0x10; Time(fields, 40, _globals.Now()); State(fields, 7);
                }
                if (_globals.Server() && elapsed > 4.0f && Read(fields.Bytes, 48) == 0)
                {
                    ushort sid = Create(fields, parent), actual = Parent(fields); ReadOnlySpan<byte> actualRaw = Pool.Slot(actual);
                    if (actualRaw[6] == 0 && actualRaw[7] == 0) Need(_hooks.Assertion)(0xa4);
                    BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(actual)[12..], sid); float x = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), y = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
                    float height = Need(_hooks.Height)(parent, BinaryPrimitives.ReadInt32LittleEndian(raw[36..])), foot = Need(_hooks.FootY)(parent); float py = (float)(((double)height * .09090909361839294 - foot) + y);
                    BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(sid)[12..], Parent(fields)); Need(_hooks.Pop)(sid, (float)((double)x - 1.399999976158142), py);
                }
                return;
            case 5: if (_globals.Now() - Time(fields, 40) > 26.0) State(fields, 1); return;
            case 11: fields.Bytes[8] |= 2; Time(fields, 16, _globals.Now()); State(fields, 12); Flyer(fields, 0); return;
            case 12:
                if (_globals.Server() && Read(fields.Bytes, 48) == 0) { ushort sid = Create(fields, parent); (float x, float y) = Need(_hooks.CirclePosition)(parent, sid, fields); BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(sid)[12..], Parent(fields)); Need(_hooks.Pop)(sid, x, y); }
                if (Read(fields.Bytes, 48) != 0 || !_globals.Boss()) { if (_globals.Server()) { Need(_hooks.CreateFlyerProcess)(Flyer(fields), Parent(fields)); Flyer(fields, 0); } State(fields, 13); } return;
            case 13: Need(_hooks.Finish)(fields); return;
            default: if (_globals.Checking.IsEnabled && (stage < 1 || stage > 13)) Need(_hooks.Assertion)(0x8da); return;
        }
    }
}
