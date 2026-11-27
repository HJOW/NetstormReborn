// 내장 모듈만 사용하는 정적 웹 서버
const http = require('http');
const fs = require('fs');
const path = require('path');

// 서버가 사용할 포트 번호
const PORT = 9803;

// 웹 소스 루트 경로 (server.js 와 같은 위치의 www 디렉토리)
const WWW_ROOT = path.join(__dirname, 'www');

// 확장자별 Content-Type 매핑
const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.htm': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.txt': 'text/plain; charset=utf-8',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.gif': 'image/gif',
  '.bmp': 'image/bmp',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.wav': 'audio/wav',
  '.mp3': 'audio/mpeg',
  '.ogg': 'audio/ogg',
  '.mp4': 'video/mp4',
  '.webm': 'video/webm',
  '.ttf': 'font/ttf',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
  '.wasm': 'application/wasm'
};

// 오류 응답 전송 함수
function sendError(res, status, message) {
  res.writeHead(status, { 'Content-Type': 'text/plain; charset=utf-8' });
  res.end(message);
}

// 같은 URL의 배포 교체를 다시 확인하게 하는 약한 검증자다. 파일 내용 전체를 매번 읽지 않고 크기·수정/상태변경 시각·파일번호를 사용한다.
function cacheHeaders(stat) {
  return {
    'Cache-Control': 'no-cache',
    ETag: `W/"${stat.size.toString(16)}-${stat.mtimeMs.toString(16)}-${stat.ctimeMs.toString(16)}-${stat.ino.toString(16)}"`,
    'Last-Modified': stat.mtime.toUTCString()
  };
}

// GET/HEAD의 조건부 검증이다. ETag가 있으면 날짜보다 우선하며, 쉼표를 포함한 다른 따옴표 값도 하나의 태그로 읽는다.
function notModified(req, stat, headers) {
  const supplied = req.headers['if-none-match'];
  if (supplied !== undefined) {
    if (supplied.trim() === '*') return true;
    const current = headers.ETag.slice(2);
    const tags = supplied.match(/(?:W\/)?"[\x21\x23-\x7e\x80-\xff]*"|[^,\s]+/g) || [];
    return tags.some((tag) => tag.replace(/^W\//, '') === current);
  }
  const since = Date.parse(req.headers['if-modified-since'] || '');
  return Number.isFinite(since) && Math.floor(stat.mtimeMs / 1000) * 1000 <= since;
}

// 파일 전송 함수
function serveFile(req, res, filePath, err, stat) {
  if (err || !stat.isFile()) return sendError(res, 404, '404 Not Found');

  const headers = cacheHeaders(stat);
  if (notModified(req, stat, headers)) {
    res.writeHead(304, headers);
    return res.end();
  }
  const ext = path.extname(filePath).toLowerCase();
  res.writeHead(200, {
    ...headers,
    'Content-Type': MIME_TYPES[ext] || 'application/octet-stream',
    'Content-Length': stat.size
  });
  if (req.method === 'HEAD') return res.end();

  // 스트림으로 전송
  const stream = fs.createReadStream(filePath);
  stream.on('error', () => res.destroy());
  stream.pipe(res);
}

// 정적 요청을 처리한다. mountPath는 끝에 슬래시가 붙은 공개 경로다. (2026-11-19 WB04-E945)
function serveRequest(req, res, mountPath, rootDirectory) {
  // GET, HEAD 이외의 메서드는 허용하지 않음
  if (req.method !== 'GET' && req.method !== 'HEAD') {
    return sendError(res, 405, '405 Method Not Allowed');
  }

  // 쿼리스트링 제거 및 URL 디코딩
  let urlPath;
  try {
    urlPath = decodeURIComponent(req.url.split('?')[0].split('#')[0]);
  } catch (e) {
    return sendError(res, 400, '400 Bad Request');
  }

  // 널 문자 차단
  if (urlPath.includes('\0')) return sendError(res, 400, '400 Bad Request');

  // 공개 경로 아래의 요청을 웹 루트에 매핑한 뒤 웹 루트 밖 접근을 차단한다.
  if (!urlPath.startsWith(mountPath)) return sendError(res, 404, '404 Not Found');
  let filePath = path.normalize(path.join(rootDirectory, urlPath.slice(mountPath.length)));
  if (filePath !== rootDirectory && !filePath.startsWith(rootDirectory + path.sep)) {
    return sendError(res, 403, '403 Forbidden');
  }

  fs.stat(filePath, (err, stat) => {
    // 디렉토리면 index.html 을 제공
    if (!err && stat.isDirectory()) {
      filePath = path.join(filePath, 'index.html');
      return fs.stat(filePath, (err2, stat2) => serveFile(req, res, filePath, err2, stat2));
    }
    serveFile(req, res, filePath, err, stat);
  });
}

// 웹 루트를 주어진 URL 경로 아래 제공한다. 포트/주소는 호출자가 정하며 rootDirectory는 분리된 배포 사본 검사용이다(기본 www).
function createStaticServer(mountPath = '/', { rootDirectory = WWW_ROOT } = {}) {
  if (!mountPath.startsWith('/') || !mountPath.endsWith('/')) {
    throw new RangeError('공개 경로는 슬래시로 시작하고 끝나야 합니다.');
  }
  const root = path.resolve(rootDirectory);
  return http.createServer((req, res) => serveRequest(req, res, mountPath, root));
}

if (require.main === module) {
  // 기존 개발 실행은 www 디렉토리를 보장하고 기본 경로로 시작한다.
  fs.mkdirSync(WWW_ROOT, { recursive: true });
  createStaticServer().listen(PORT, () => {
    console.log(`Server started: http://localhost:${PORT}/ (root: ${WWW_ROOT})`);
  });
}

module.exports = { createStaticServer };
