// 고정한 본 소스 대신 스커미시 시작 대본 후보 한 모듈만 읽는다.
import {registerHooks} from 'node:module';
import {readFileSync} from 'node:fs';
registerHooks({load(url,context,nextLoad){if(url.endsWith('/www/core/skirmish-setup.js'))return{format:'module',source:readFileSync(new URL('skirmish-setup.js',import.meta.url),'utf8'),shortCircuit:true};return nextLoad(url,context);}});
