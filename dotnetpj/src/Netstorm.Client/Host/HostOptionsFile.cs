using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>cpppj ClientOptionsFile의 원본 CP1252 및 Unicode 경로 저장 경계다.</summary>
public static class HostOptionsFile
{
    private static readonly UTF8Encoding StrictUtf8 = new(false, true);

    /// <summary>표현 가능한 원본은 원래 바이트를 사용하고 나머지는 서명 뒤 UTF-8 BOM을 넣어 XOR한다.</summary>
    public static byte[] Encode(string text)
    {
        try { return ConfigInterface.EncodeConfigFile(text); }
        catch (InvalidDataException)
        {
            if (!text.StartsWith(ConfigTextOps.Signature, StringComparison.Ordinal)) throw;
            string body = text[ConfigTextOps.Signature.Length..];
            if (body.StartsWith('\ufeff')) body = body[1..];
            byte[] encoded = [.. Encoding.ASCII.GetBytes(ConfigTextOps.Signature), 0xef, 0xbb, 0xbf, .. StrictUtf8.GetBytes(body)];
            XorCipher.Apply(encoded); return encoded;
        }
    }

    /// <summary>실제 쓰기 실패는 재시도하지 않고 기존 변경 표시를 보존한다.</summary>
    public static void Save(ConfigInterface config, Action<string, byte[]> writer)
    {
        ArgumentNullException.ThrowIfNull(config); ArgumentNullException.ThrowIfNull(writer);
        bool changed = config.Configuration.Changed;
        try
        {
            byte[] bytes = Encode(config.SaveText("d\\options.cfg")); config.Configuration.Changed = changed;
            writer("d\\options.cfg", bytes); config.Configuration.Changed = false;
        }
        catch { config.Configuration.Changed = changed; throw; }
    }
}
