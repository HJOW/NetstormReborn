// 원본 기계어 관측 13행과 실제 부모의 부분 실패를 검사한다.
using System.Globalization;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawFortPriestPreparation의 기술·지역 사제 준비를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class FortPriestPreparationTests
{
    /// <summary>검사 풀 용량이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Capacity = 30000;

    /// <summary>지정 폭의 원시 필드만 변경한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int at, uint value)
    {
        // 주변 필드는 그대로 두는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            raw[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>부모·두 도우미·안정 제거·테마 타입을 실제 관측 13행과 대조한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void Native13ActualParentAndListHelpers()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "fort-priest-preparation-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int rows = 0;
        // 주석 외 모든 최초 관측 행을 한 번씩 독립 물리에서 재현하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string[] row = line.Split('\t');
            Assert.Equal(3, row.Length);
            string label = row[0];
            string expected = row[1];
            string post = row[2];
            RawSidPool pool = new(Capacity);
            PriestPostPopState priests = new();
            FortPriestPreparationGlobals globals = new()
            {
                MissionPost = label == "mission",
                Editor = label == "editor" ? 1u : 0u,
                Subscriber = 5u,
                TemplateOwner = 1u,
                PlayerOwner = 7u,
            };
            uint finds = 0u;
            byte[] state = new byte[28];
            state[17] = label == "state17" ? (byte)1 : (byte)0;
            List<string> events = [];
            if (label == "duplicates")
            {
                priests.Priests.Entries = [1u, 1u, 2u];
            }
            else if (label == "full-capacity")
            {
                uint[] full = new uint[22];
                // 고정 20칸 출력은 더 큰 원문 목록도 끝까지 조회하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
                for (uint value = 1; value <= 22; ++value)
                {
                    full[(int)value - 1] = value;
                }

                priests.Priests.Entries = full;
            }
            else if (label != "populate")
            {
                priests.Priests.Entries = [1u, 2u, 3u];
            }

            priests.Priests.Count = (uint)priests.Priests.Entries.Length;
            // 검사 소유의 실제 풀에 원시 입력만 설치하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (uint sid = 1; sid <= 22; ++sid)
            {
                Span<byte> raw = pool.RawSlot((int)sid);
                Put(raw, 14, BitConverter.SingleToUInt32Bits((float)sid));
                Put(raw, 18, BitConverter.SingleToUInt32Bits(1.0f));
                raw[12] = ((label == "marked" && sid == 2) || (label == "duplicates" && sid == 1)) ? (byte)0x80 : (byte)0;
            }

            FortPriestPreparationHooks hooks = new()
            {
                BaseType = index => index == 0 ? 133u : index == 1 ? 71u : 128u,
                ThemeType = theme =>
                {
                    Assert.Equal(2u, theme);
                    return 77u;
                },
                Technology = () =>
                {
                    events.Add("tech");
                    return 4u;
                },
                Contains = (parent, type) =>
                {
                    Assert.Equal(4u, parent);
                    events.Add("find:" + type.ToString(CultureInfo.InvariantCulture));
                    ++finds;
                    if (label == "post-change" && finds == 1)
                    {
                        globals.MissionPost = true;
                    }

                    return finds % 2 == 0 ? type : 0u;
                },
                CreateTechnology = (parent, type) =>
                {
                    Assert.Equal(4u, parent);
                    events.Add("create:" + type.ToString(CultureInfo.InvariantCulture));
                },
                TemplatePresent = owner =>
                {
                    Assert.Equal(1u, owner);
                    return true;
                },
                State = owner =>
                {
                    Assert.Equal(1u, owner);
                    return state;
                },
                Territory = (owner, output) =>
                {
                    Assert.Equal(1u, owner);
                    events.Add("territory");
                    Array.Clear(output);
                    if (label != "no-record")
                    {
                        output[1] = 1;
                    }

                    if (label == "two-regions")
                    {
                        output[7] = 1;
                    }
                },
                Island = (owner, index) =>
                {
                    Assert.Equal(7u, owner);
                    events.Add("island:" + index.ToString(CultureInfo.InvariantCulture));
                    return label == "missing-island" ? 127 : (int)index + 1;
                },
                Region = (x, y) =>
                {
                    Assert.Equal(1.0f, y);
                    if (label == "shorten-list")
                    {
                        priests.GetPriests().Count = 1u;
                    }

                    return label == "two-regions" && x == 3.0f ? 2 : 1;
                },
                SetOwner = (sid, owner) =>
                {
                    events.Add("owner:" + sid.ToString(CultureInfo.InvariantCulture) + ":" + owner.ToString(CultureInfo.InvariantCulture));
                    pool.RawSlot((int)sid)[34] = (byte)owner;
                },
                Populate = region => events.Add("populate:" + region.ToString(CultureInfo.InvariantCulture)),
                TellPriests = count => events.Add("tell:" + count.ToString(CultureInfo.InvariantCulture)),
            };
            new RawFortPriestPreparation(pool, priests, globals, hooks).Run();
            string actual = events.Count == 0 ? string.Empty : string.Join(';', events);
            Assert.Equal(expected, actual);
            Assert.Equal(post, (globals.MissionPost ? 1 : 0).ToString(CultureInfo.InvariantCulture));
            ++rows;
        }

        Assert.Equal(13, rows);
    }

    /// <summary>선택된 기술 생성의 예외는 앞선 효과를 보존하고 뒤 장치 누락을 미리 요구하지 않는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectedFailureKeepsEarlierTechnologyEffects()
    {
        RawSidPool pool = new(Capacity);
        PriestPostPopState priests = new();
        FortPriestPreparationGlobals globals = new()
        {
            MissionPost = false,
            Editor = 1u,
            Subscriber = 0u,
            TemplateOwner = 1u,
            PlayerOwner = 1u,
        };
        uint reads = 0u;
        uint created = 0u;
        FortPriestPreparationHooks hooks = new()
        {
            BaseType = index =>
            {
                ++reads;
                return 100u + index;
            },
            Technology = () => 1u,
            Contains = (parent, type) => 0u,
            CreateTechnology = (parent, type) =>
            {
                ++created;
                if (type == 101u)
                {
                    throw new InvalidOperationException("선택한 두 번째 기술 생성 중단");
                }
            },
        };
        bool stopped = false;
        try
        {
            new RawFortPriestPreparation(pool, priests, globals, hooks).Run();
        }
        catch (InvalidOperationException error)
        {
            stopped = error.Message == "선택한 두 번째 기술 생성 중단";
        }

        Assert.True(stopped);
        Assert.Equal(2u, reads);
        Assert.Equal(2u, created);
        Assert.False(globals.MissionPost);
        Assert.Equal(1u, globals.Editor);
    }
}
