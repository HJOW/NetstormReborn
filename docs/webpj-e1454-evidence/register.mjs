// 고정 중인 본 소스 대신 표시 후보 두 모듈만 읽고 다른 import는 원래 경로를 유지한다.
import {registerHooks} from 'node:module';
import {readFileSync} from 'node:fs';
registerHooks({load(url,context,nextLoad){for(const [suffix,file]of [['/www/netstorm.js','netstorm.js'],['/www/render/presenter.js','presenter.js'],['/www/render/screen.js','screen.js'],['/www/core/raw-surface-world.js','raw-surface-world.js']])if(url.endsWith(suffix))return{format:'module',source:readFileSync(new URL(file,import.meta.url),'utf8'),shortCircuit:true};return nextLoad(url,context);}});
