using Netstorm.Assets;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// 미션을 골라 요새 원문을 읽고 이식된 판독기로 푼 실제 적재 결과다.
public sealed record MissionFortLoadResult(
    // 고른 미션 이름이다.
    string Mission,
    // 미션이 고른 논리 요새 이름이다.
    string FortName,
    // 읽은 요새 파일 경로(`fortSpec`)다.
    string FortPath,
    // 미션 종류(`missionType`)다.
    string MissionType,
    // 푼 요새 Template다.
    FortTemplate Fort);

/// <summary>호스트에서 미션 요새를 실제 파일 장치로 읽어 이식된 판독기로 푸는 적재 장치다.
/// 근거: cpppj의 미션 진입 요새 적재(<c>MissionScript::FortPath</c> + <c>FortTemplate::Parse</c>, R4 조립의 첫 고리).
/// (2026-11-12 0.391에서 R4 실제 호스트·월드 조립을 시작하려고 추가했다)</summary>
public static class MissionFortLoad
{
    /// <summary>게임 파일 장치에서 10.78 타입 표를 로딩 순서대로 읽어 만든다.</summary>
    /// <param name="files">게임 데이터 파일 장치.</param>
    /// <returns>요새 풀기에 쓰는 판본 타입 표.</returns>
    /// <remarks>호스트는 10.78만 다루므로 판본을 고정한다. .type 원문 읽기는
    /// <c>RiftTypeOriginalFilesTests</c>와 같은 규칙(BOM이면 UTF-8, 아니면 Windows-1252)이다.</remarks>
    public static RiftTypeTable LoadTypeTable(GameFileSystem files)
    {
        ArgumentNullException.ThrowIfNull(files);
        const OriginalEdition edition = OriginalEdition.Patch1078;
        IReadOnlyList<string> order = RiftTypeLoadOrder.Get(edition);
        var sources = new RiftTypeSource[order.Count];
        // 로딩 순서의 이름마다 d/이름.type을 읽는다.
        for (int index = 0; index < order.Count; index++)
        {
            byte[]? bytes = files.TryReadAllBytes("d/" + order[index] + ".type");
            if (bytes is null) throw new InvalidDataException("타입 원문이 없다: d/" + order[index] + ".type");
            sources[index] = new RiftTypeSource(order[index], RiftTypeDefinition.Parse(DecodeType(bytes)));
        }
        return new RiftTypeTable(edition, sources);
    }

    /// <summary>원문 바이트를 cpppj <c>DecodeOriginalText</c>와 같이 읽는다.</summary>
    /// <param name="bytes">.type 파일 바이트.</param>
    private static string DecodeType(byte[] bytes) =>
        bytes.AsSpan().StartsWith((ReadOnlySpan<byte>)[0xEF, 0xBB, 0xBF])
            ? System.Text.Encoding.UTF8.GetString(bytes, 3, bytes.Length - 3)
            : OriginalText.DecodeWindows1252(bytes);

    /// <summary>시작 화면의 같은 설정·파일 장치로 미션 스크립트를 열고 요새를 읽어 푼다.</summary>
    /// <param name="startup">같은 파일 시스템과 Config를 보유한 시작 화면.</param>
    /// <param name="mission">고를 미션 이름.</param>
    /// <returns>스크립트 정보와 푼 Template.</returns>
    public static MissionFortLoadResult Load(HostStartupScene startup, string mission)
    {
        ArgumentNullException.ThrowIfNull(startup);
        ArgumentException.ThrowIfNullOrEmpty(mission);
        using var script = new Netstorm.Client.Mission.MissionScript(startup.Configuration, mission);
        if (!script.Loaded) throw new InvalidDataException("미션 스크립트가 없다: " + mission);
        string fortName = script.FortName();
        MissionFortSourceImage image = script.FortFileReader()(fortName);
        RiftTypeTable types = LoadTypeTable(startup.Files);
        FortTemplate fort = FortTemplate.Parse(image.Image, types);
        return new MissionFortLoadResult(mission, fortName, image.FileName, script.MissionType(), fort);
    }
}
