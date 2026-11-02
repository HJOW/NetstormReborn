using System.Globalization;

namespace Netstorm.Core.Original;

/// <summary>
/// 타입 정의(.type)의 속성 하나다. 이름과, 숫자 또는 따옴표 문자열 값 가운데 하나를 가진다(cpppj <c>TypeProperty</c>의 <c>std::variant&lt;double, std::string&gt;</c>).
/// 숫자와 문자열은 서로 변환하지 않고 구분해 보존한다.
/// </summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class TypeProperty
{
    /// <summary>숫자 값 속성을 만든다.</summary>
    /// <param name="name">속성 이름.</param>
    /// <param name="number">숫자 값.</param>
    public TypeProperty(string name, double number)
    {
        Name = name;
        Number = number;
    }

    /// <summary>문자열 값 속성을 만든다.</summary>
    /// <param name="name">속성 이름.</param>
    /// <param name="text">따옴표 안의 문자열(원문 그대로, 이스케이프 해석 없음).</param>
    public TypeProperty(string name, string text)
    {
        Name = name;
        Text = text;
    }

    /// <summary>이름으로 조회·표시할 때 쓰는 속성 이름이다. 비교는 ASCII 대소문자를 구분하지 않는다.</summary>
    public string Name { get; }

    /// <summary>숫자 값이다. 문자열 속성이면 null이다.</summary>
    public double? Number { get; }

    /// <summary>문자열 값이다. 숫자 속성이면 null이다.</summary>
    public string? Text { get; }
}

/// <summary>프레임 클러스터가 참조하는 이미지 파일과 번호다. GIF 번호는 빌드용 참조이며 런타임 SHP 선택에는 쓰지 않는다.</summary>
/// <param name="File">읽기·재생에 쓰는 파일 이름.</param>
/// <param name="Number">그 파일 안의 번호.</param>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed record TypeImageReference(string File, double Number);

/// <summary>같은 타입의 프레임 클러스터 하나다: 이름, 프레임 코드, 이미지 목록. 선언 순서가 SHP 레이어별 프레임 순서이므로 정렬하지 않는다.</summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class TypeCluster
{
    /// <summary>클러스터 이름이다(예: <c>A00</c>, <c>AA260</c>).</summary>
    public string Name { get; set; } = "";

    /// <summary>이름과 플래그 단어에서 만든 프레임 코드다.</summary>
    public RiftFrameCode Code { get; set; }

    /// <summary>레이어별 이미지 참조다.</summary>
    public List<TypeImageReference> Images { get; } = [];
}

/// <summary>특수 용도로 선택되는 타입 프레임 번호 묶음이다.</summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class TypeSpecialFrames
{
    /// <summary>기본 프레임 번호다. 빈 타입의 초기값은 0이다.</summary>
    public int DefaultFrame { get; set; }

    /// <summary>도움말 그림용 프레임 번호다. 빈 타입의 초기값은 0이며, .type을 읽을 때는 먼저 -1로 쓴다.</summary>
    public int GumpFrame { get; set; }

    /// <summary>도움말 프레임 번호다. 원본의 초기값은 -1이다.</summary>
    public int HelpFrame { get; set; } = -1;

    /// <summary>받침 프레임 번호다. 원본의 초기값은 -1이다.</summary>
    public int BaseFrame { get; set; } = -1;
}

/// <summary>
/// 타입 한 개의 자산 정의(.type 문법)다(cpppj <c>o/TypeParser.cpp</c>의 <c>RiftTypeDefinition</c>). 모든 게임 플래그·생성자 함수 포인터를 복원한 구조체는 아니다.
/// 속성은 원문 순서와 중복을 보존하고 조회할 때 마지막 지정값이 이긴다. 클러스터에서 프레임 코드와 특수 프레임 번호를 만든다(원본 0049c3b0 ↔ CD 004460f0).
/// </summary>
/// <remarks>
/// 2026-11-02 F1에서 추가했다. 입력은 .NET 문자열이며 원본 바이트를 문자 하나 = 바이트 하나로 풀어 낸 것과 같다.
/// 기존 경로의 <c>Netstorm.Assets.TypeDefinition</c>(속성을 문자열로만 들고 있다)은 교체 대상이라 연결하지 않는다.
/// </remarks>
public sealed class RiftTypeDefinition
{
    /// <summary>타입 이름(<c>typename</c> 뒤 단어)이다. 타입 표의 이름은 이 값이 아니라 파일 이름이다.</summary>
    public string Name { get; set; } = "";

    /// <summary>타입 이름 뒤의 수식어(<c>constructor</c> / <c>client constructor</c> 등)다. 없으면 빈 문자열이다.</summary>
    public string Constructor { get; set; } = "";

    /// <summary>동작을 선택하는 플래그 단어(<c>typeflags</c> 뒤)다. 이름 목록을 먼저 보존하고 비트 매핑은 타입 표가 한다.</summary>
    public List<string> Flags { get; } = [];

    /// <summary>본문 속성이다. 원문 순서와 중복을 그대로 둔다.</summary>
    public List<TypeProperty> Properties { get; } = [];

    /// <summary>프레임 클러스터다. 선언 순서를 보존한다.</summary>
    public List<TypeCluster> Clusters { get; } = [];

    /// <summary>특수 동작에서 선택할 프레임 번호 묶음이다.</summary>
    public TypeSpecialFrames SpecialFrames { get; } = new();

    /// <summary>자산 문법의 토큰 종류다: 식별자·문자열·숫자·구분자·파일 끝.</summary>
    private enum TokenKind
    {
        /// <summary>영문자·밑줄로 시작하는 식별자(숫자 포함 가능)다.</summary>
        Word,

        /// <summary>따옴표 문자열이다.</summary>
        String,

        /// <summary>숫자다.</summary>
        Number,

        /// <summary>한 글자 구분자다.</summary>
        Punctuation,

        /// <summary>입력의 끝이다.</summary>
        End,
    }

    /// <summary>타입 정의 해석에 쓰는 토큰의 종류·문자열·숫자 값이다.</summary>
    /// <param name="Kind">토큰 종류.</param>
    /// <param name="Text">토큰 원문(문자열 토큰은 따옴표 안쪽).</param>
    /// <param name="Number">숫자 토큰의 값.</param>
    private readonly record struct Token(TokenKind Kind, string Text, double Number);

    /// <summary>ASCII 대소문자를 무시한 비교다(원본 stricmp).</summary>
    /// <param name="left">왼쪽 문자열.</param>
    /// <param name="right">오른쪽 문자열.</param>
    internal static bool AsciiEqual(string left, string right)
    {
        if (left.Length != right.Length)
        {
            return false;
        }
        // 글자마다 ASCII 소문자로 접어 비교한다(비ASCII 글자는 그대로 비교한다)
        for (int index = 0; index < left.Length; index++)
        {
            if (AsciiLower(left[index]) != AsciiLower(right[index]))
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>ASCII 대문자 한 글자를 소문자로 바꾼다.</summary>
    /// <param name="value">바꿀 글자.</param>
    private static char AsciiLower(char value) => value is >= 'A' and <= 'Z' ? (char)(value + 32) : value;

    /// <summary>스크립트 식별자의 영문자·밑줄인지 판정한다.</summary>
    /// <param name="value">검사할 글자.</param>
    private static bool Letter(char value) => value is (>= 'A' and <= 'Z') or (>= 'a' and <= 'z') or '_';

    /// <summary>숫자는 원본처럼 십진수로 읽으며 00도 8진수로 바꾸지 않는다.</summary>
    /// <param name="value">검사할 글자.</param>
    private static bool Digit(char value) => value is >= '0' and <= '9';

    /// <summary>
    /// <c>std::from_chars(…, double, general)</c>와 같은 문법으로 글머리의 실수를 읽는다: 선택적 '-', 정수부와 선택적 소수부(또는 소수점으로 시작하는 소수부),
    /// 지수는 뒤에 숫자가 이어질 때만 소비한다. 앞 공백·'+'·16진수·무한대·NaN 표기는 받지 않는다.
    /// </summary>
    /// <param name="text">읽을 문자열.</param>
    /// <param name="start">읽기 시작 위치.</param>
    /// <param name="end">이 위치까지만 읽는다.</param>
    /// <param name="length">읽은 글자 수(실패하면 0).</param>
    /// <param name="value">읽은 값. 범위를 넘으면 무한대이며 호출자가 거부한다.</param>
    /// <returns>숫자를 하나라도 읽었으면 true.</returns>
    private static bool ScanNumber(string text, int start, int end, out int length, out double value)
    {
        int position = start;
        if (position < end && text[position] == '-')
        {
            position++;
        }
        int integerDigits = 0;
        // 정수부
        while (position < end && Digit(text[position]))
        {
            position++;
            integerDigits++;
        }
        int fractionDigits = 0;
        if (position < end && text[position] == '.')
        {
            int afterDot = position + 1;
            // 소수부
            while (afterDot < end && Digit(text[afterDot]))
            {
                afterDot++;
                fractionDigits++;
            }
            // "5."는 소수점까지, ".5"는 숫자가 있을 때만 읽는다
            if (integerDigits > 0 || fractionDigits > 0)
            {
                position = afterDot;
            }
        }
        if (integerDigits == 0 && fractionDigits == 0)
        {
            length = 0;
            value = 0;
            return false;
        }
        if (position < end && (text[position] == 'e' || text[position] == 'E'))
        {
            int exponent = position + 1;
            if (exponent < end && (text[exponent] == '+' || text[exponent] == '-'))
            {
                exponent++;
            }
            int exponentDigits = 0;
            // 지수부는 숫자가 이어질 때만 소비한다
            while (exponent < end && Digit(text[exponent]))
            {
                exponent++;
                exponentDigits++;
            }
            if (exponentDigits > 0)
            {
                position = exponent;
            }
        }
        length = position - start;
        value = double.Parse(text.AsSpan(start, length), NumberStyles.Float, CultureInfo.InvariantCulture);
        return true;
    }

    /// <summary>주석은 문자열 바깥에서만 제거한다. 문자열 안의 경로·//·세미콜론은 그대로 둔다.</summary>
    private sealed class TypeLexer
    {
        /// <summary>해석할 원문이다(NUL 앞까지).</summary>
        private readonly string _text;

        /// <summary>현재 읽기 위치다.</summary>
        private int _position;

        /// <summary>바로 앞에서 읽은 현재 토큰이다.</summary>
        private Token _token;

        /// <summary>파싱 동안만 원문을 참조하고 NUL 뒤의 데이터는 읽지 않는다. 첫 토큰을 읽어 둔다.</summary>
        /// <param name="text">.type 원문.</param>
        public TypeLexer(string text)
        {
            int nul = text.IndexOf('\0');
            _text = nul < 0 ? text : text[..nul];
            Next();
        }

        /// <summary>현재 토큰이다.</summary>
        public Token Current => _token;

        /// <summary>예상한 문장부호를 소비하거나 잘못된 문법을 보고한다.</summary>
        /// <param name="punctuation">기대하는 한 글자 구분자.</param>
        public void Expect(string punctuation)
        {
            if (_token.Kind != TokenKind.Punctuation || _token.Text != punctuation)
            {
                Fail();
            }
            Next();
        }

        /// <summary>식별자 또는 문자열을 소비해 값으로 돌려준다.</summary>
        /// <param name="kind">기대하는 토큰 종류.</param>
        public string Take(TokenKind kind)
        {
            if (_token.Kind != kind)
            {
                Fail();
            }
            string result = _token.Text;
            Next();
            return result;
        }

        /// <summary>숫자 토큰을 소비해 값으로 돌려준다.</summary>
        public double TakeNumber()
        {
            if (_token.Kind != TokenKind.Number)
            {
                Fail();
            }
            double result = _token.Number;
            Next();
            return result;
        }

        /// <summary>다음 토큰을 읽고 닫히지 않은 문자열·잘못된 숫자를 거부한다.</summary>
        public void Next()
        {
            SkipSpace();
            if (_position == _text.Length)
            {
                _token = new Token(TokenKind.End, "", 0);
                return;
            }
            char current = _text[_position];
            if (current == '"')
            {
                int start = ++_position;
                // 원본 자산의 문자열에는 Windows 역슬래시가 있으므로 C 이스케이프로 해석하지 않는다
                while (_position < _text.Length && _text[_position] != '"')
                {
                    _position++;
                }
                if (_position == _text.Length)
                {
                    Fail();
                }
                _token = new Token(TokenKind.String, _text[start.._position], 0);
                _position++;
            }
            else if (Letter(current))
            {
                int start = _position++;
                // A00·sunCannon·hotFootRatioX 같은 이름을 하나로 읽는다
                while (_position < _text.Length && (Letter(_text[_position]) || Digit(_text[_position])))
                {
                    _position++;
                }
                _token = new Token(TokenKind.Word, _text[start.._position], 0);
            }
            else if (Digit(current) || current == '-' || current == '+' || current == '.')
            {
                int start = _position;
                // from_chars는 앞의 '+'를 받지 않으므로 원본처럼 하나만 건너뛰고 읽는다
                int numeric = start + (current == '+' ? 1 : 0);
                if (!ScanNumber(_text, numeric, _text.Length, out int length, out double value) || !double.IsFinite(value))
                {
                    Fail();
                }
                _position = numeric + length;
                _token = new Token(TokenKind.Number, _text[start.._position], value);
            }
            else
            {
                _token = new Token(TokenKind.Punctuation, current.ToString(), 0);
                _position++;
            }
        }

        /// <summary>행 끝 문자를 포함한 공백과 // 주석을 건너뛰어 다음 토큰의 위치로 이동한다.</summary>
        private void SkipSpace()
        {
            // 주석을 건너뛴 뒤에도 연속 공백·주석이 나올 수 있다
            while (_position < _text.Length)
            {
                char current = _text[_position];
                if (current is ' ' or '\t' or '\r' or '\n' or '\f' or '\v')
                {
                    _position++;
                    continue;
                }
                if (current == '/' && _position + 1 < _text.Length && _text[_position + 1] == '/')
                {
                    // CRCRLF 파일과 일반 LF 파일을 모두 읽는다
                    while (_position < _text.Length && _text[_position] != '\n')
                    {
                        _position++;
                    }
                    continue;
                }
                break;
            }
        }

        /// <summary>오류 위치를 붙여 손상된 정의 파일을 찾기 쉽게 한다.</summary>
        /// <exception cref="InvalidDataException">항상 던진다.</exception>
        [System.Diagnostics.CodeAnalysis.DoesNotReturn]
        private void Fail() => throw new InvalidDataException("Invalid .type syntax near byte " + _position.ToString(CultureInfo.InvariantCulture));
    }

    /// <summary>_ftol의 정상 범위에서 0 방향으로 버리고 원본 char 저장의 하위 8비트를 보존한다.</summary>
    /// <param name="number">클러스터 이름에서 읽은 프레임 번호.</param>
    private static byte FrameNumber(double number)
    {
        double truncated = Math.Truncate(number);
        if (truncated < int.MinValue || truncated > int.MaxValue)
        {
            throw new InvalidDataException("Frame number is outside recovered integer range");
        }
        return unchecked((byte)(int)truncated);
    }

    /// <summary>클러스터 이름을 원본의 측면·변형·번호 코드로 분리한다. 이름의 글자 접두부 뒤가 번호이며, 이름이 글자뿐이면 다음 토큰의 숫자가 번호다.</summary>
    /// <param name="label">클러스터 이름.</param>
    /// <param name="lexer">번호가 다음 토큰일 때 읽을 어휘기.</param>
    private static RiftFrameCode ReadFrameCode(string label, TypeLexer lexer)
    {
        int letters = 0;
        // 문자가 끝나는 자리부터 프레임 번호가 시작한다
        while (letters < label.Length && Letter(label[letters]))
        {
            letters++;
        }
        if (letters == 0 || label[0] < 'A' || label[0] > 'Z')
        {
            throw new InvalidDataException("Invalid frame side orientation");
        }
        double number;
        if (letters == label.Length)
        {
            number = lexer.TakeNumber();
        }
        else if (!ScanNumber(label, letters, label.Length, out int length, out number) || letters + length != label.Length)
        {
            throw new InvalidDataException("Invalid cluster frame number");
        }
        return new RiftFrameCode((byte)label[0], (byte)(letters == 1 ? 'P' : label[1]), FrameNumber(number), 0);
    }

    /// <summary>
    /// 순서·중복·미사용 플래그·GIF 참조를 보존하며 런타임 프레임 코드를 생성한다(cpppj <c>RiftTypeDefinition::Parse</c>).
    /// 잘린 문자열·구분자·방향·비유한 숫자는 모두 예외다.
    /// </summary>
    /// <param name="text">.type 원문.</param>
    /// <exception cref="InvalidDataException">문법 오류.</exception>
    public static RiftTypeDefinition Parse(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        TypeLexer lexer = new(text);
        RiftTypeDefinition type = new();
        // .type 로딩은 +11c를 -1로 먼저 쓴다. gumpframe 미지정 시 StyleText의 특수 그림은 default로 복귀한다
        type.SpecialFrames.GumpFrame = -1;
        if (!AsciiEqual(lexer.Take(TokenKind.Word), "typename"))
        {
            throw new InvalidDataException("Missing typename");
        }
        type.Name = lexer.Take(TokenKind.Word);
        // constructor / client constructor 수식어와 선택적인 typeflags를 읽는다
        while (lexer.Current.Kind == TokenKind.Word)
        {
            string word = lexer.Take(TokenKind.Word);
            if (AsciiEqual(word, "typeflags"))
            {
                // 원본 로더의 게임별 비트 매핑은 타입 표가 하며 이름 목록을 먼저 보존한다
                while (lexer.Current.Kind == TokenKind.Word)
                {
                    type.Flags.Add(lexer.Take(TokenKind.Word));
                }
                lexer.Expect(";");
            }
            else
            {
                if (type.Constructor.Length != 0)
                {
                    type.Constructor += ' ';
                }
                type.Constructor += word;
            }
        }
        lexer.Expect("{");
        // 속성은 원문 순서대로 저장하고 조회할 때 뒤의 지정값을 우선한다
        while (lexer.Current.Kind == TokenKind.Word)
        {
            string name = lexer.Take(TokenKind.Word);
            lexer.Expect("=");
            TypeProperty property = lexer.Current.Kind == TokenKind.String
                ? new TypeProperty(name, lexer.Take(TokenKind.String))
                : new TypeProperty(name, lexer.TakeNumber());
            lexer.Expect(";");
            type.Properties.Add(property);
        }
        lexer.Expect("}");
        bool hasHelp = false;
        // 선언 순서가 SHP의 레이어별 프레임 순서이므로 정렬하지 않는다
        while (lexer.Current.Kind != TokenKind.End)
        {
            TypeCluster cluster = new() { Name = lexer.Take(TokenKind.Word) };
            cluster.Code = ReadFrameCode(cluster.Name, lexer);
            lexer.Expect(":");
            int index = type.Clusters.Count;
            // 알려진 7개 비트 외의 단어는 원본처럼 프레임 비트에 반영하지 않는다
            while (lexer.Current.Kind == TokenKind.Word)
            {
                string flag = lexer.Take(TokenKind.Word);
                byte bits = 0;
                if (AsciiEqual(flag, "fringe")) { bits |= 0x01; }
                if (AsciiEqual(flag, "suck")) { bits |= 0x02; }
                if (AsciiEqual(flag, "rim")) { bits |= 0x04; }
                if (AsciiEqual(flag, "lit")) { bits |= 0x08; }
                if (AsciiEqual(flag, "unlit")) { bits |= 0x10; }
                if (AsciiEqual(flag, "cracked")) { bits |= 0x20; }
                if (AsciiEqual(flag, "hard")) { bits |= 0x40; }
                cluster.Code = cluster.Code with { Flags = (byte)(cluster.Code.Flags | bits) };
                if (AsciiEqual(flag, "baseframe")) { type.SpecialFrames.BaseFrame = index; }
                if (AsciiEqual(flag, "gumpframe")) { type.SpecialFrames.GumpFrame = index; }
                if (AsciiEqual(flag, "default"))
                {
                    type.SpecialFrames.DefaultFrame = index;
                    if (!hasHelp)
                    {
                        type.SpecialFrames.HelpFrame = index;
                    }
                }
                if (AsciiEqual(flag, "help"))
                {
                    type.SpecialFrames.HelpFrame = index;
                    hasHelp = true;
                }
            }
            // GIF 이름은 참조만 기록하고 실제 그림은 원본 SHP 인덱스로 선택한다
            do
            {
                lexer.Expect(":");
                string file = lexer.Take(TokenKind.String);
                lexer.Expect("#");
                cluster.Images.Add(new TypeImageReference(file, lexer.TakeNumber()));
            }
            while (lexer.Current.Kind == TokenKind.Punctuation && lexer.Current.Text == ":");
            lexer.Expect(";");
            type.Clusters.Add(cluster);
        }
        return type;
    }

    /// <summary>같은 이름의 마지막 지정 속성을 찾는다. 원본 속성 이름은 ASCII 대소문자를 구분하지 않고, 속성을 순서대로 덮어쓰는 효과를 보존한다.</summary>
    /// <param name="name">속성 이름.</param>
    /// <returns>속성, 없으면 null.</returns>
    public TypeProperty? Property(string name)
    {
        // 뒤에서 찾으면 같은 속성의 마지막 지정값을 얻는다
        for (int index = Properties.Count - 1; index >= 0; index--)
        {
            if (AsciiEqual(Properties[index].Name, name))
            {
                return Properties[index];
            }
        }
        return null;
    }

    /// <summary>숫자형 속성을 조회한다. 누락이거나 문자열형이면 null이다(자동 수치 변환을 하지 않는다).</summary>
    /// <param name="name">속성 이름.</param>
    public double? Number(string name) => Property(name)?.Number;

    /// <summary>문자열형 속성을 조회한다. 누락이거나 숫자형이면 null이다(자동 문자열 변환을 하지 않는다).</summary>
    /// <param name="name">속성 이름.</param>
    public string? String(string name) => Property(name)?.Text;

    /// <summary>클러스터 순서를 프레임 검색 클래스에 연결한다. 각 클러스터가 원본 코드 배열의 원소 하나에 해당한다.</summary>
    public RiftTypeFrames FrameTable() => new(Clusters.Select(cluster => cluster.Code));

    /// <summary>자산에 저장된 숫자와 실제 배치 격자를 구분한 가로·세로 칸 수다. 원본 후처리 0049b0d0의 foot_y 6 → 8을 반영한다.</summary>
    /// <exception cref="InvalidDataException">값이 32비트 정수 범위를 넘는다.</exception>
    public int[] Footprint()
    {
        int[] result = new int[2];
        // 정상 _ftol 범위의 두 축을 원본처럼 버림 변환한다
        for (int axis = 0; axis < result.Length; axis++)
        {
            double number = Math.Truncate(Number(axis == 0 ? "foot_x" : "foot_y") ?? 0);
            if (number < int.MinValue || number > int.MaxValue)
            {
                throw new InvalidDataException("Footprint is outside integer range");
            }
            result[axis] = (int)number;
        }
        if (result[1] == 6)
        {
            result[1] = 8;
        }
        return result;
    }
}
