using Netstorm.Assets;
using Netstorm.Core.Original;

// 값 decoder와 같은 현재 Screen 저장소·적용 후단에10.78의 실제 파일 수명을 연결한다.
namespace Netstorm.Client.Display;

/// <summary>현재 파일에서 저장 팔레트로 직접 읽는 입구다. 메뉴/날씨/커서 소비자 갱신은 별도 부모다.</summary>
public sealed partial class Screen
{
    /// <summary>현재 FILE/키를 연 채 같은 저장1024BYTE에 읽고 clear/적용/색표 후단을 끝낸 뒤 닫는다.</summary>
    public void LoadNativePaletteFile(GameFileSystem files, string name, uint clear, Func<IRawBaseFileDevice> devices, Action<uint>? assertion = null)
        => PaletteFileRead.ReadNative(files, name, _saved, () => ApplySavedPalette(clear, true), devices, assertion);
}
