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

// 파일 전송 함수
function serveFile(req, res, filePath, err, stat) {
  if (err || !stat.isFile()) return sendError(res, 404, '404 Not Found');

  const ext = path.extname(filePath).toLowerCase();
  res.writeHead(200, {
    'Content-Type': MIME_TYPES[ext] || 'application/octet-stream',
    'Content-Length': stat.size
  });
  if (req.method === 'HEAD') return res.end();

  // 스트림으로 전송
  const stream = fs.createReadStream(filePath);
  stream.on('error', () => res.destroy());
  stream.pipe(res);
}

const server = http.createServer((req, res) => {
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

  // 경로 정규화 후 www 외부 접근(디렉토리 탈출) 차단
  let filePath = path.normalize(path.join(WWW_ROOT, urlPath));
  if (filePath !== WWW_ROOT && !filePath.startsWith(WWW_ROOT + path.sep)) {
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
});

// www 디렉토리가 없으면 생성
fs.mkdirSync(WWW_ROOT, { recursive: true });

server.listen(PORT, () => {
  console.log(`Server started: http://localhost:${PORT}/ (root: ${WWW_ROOT})`);
});