// Impl 안에서 같은 현재 지면/섬 신전·TYPE 속도/게임 난수와 실제54 경로를 연결한다.
void RunPriestPanic(Sid sid) const { if (!priestPanic) throw std::logic_error("사제 panic 실제 설치 전 호출");priestPanic->Run(sid); } // 세 원본 호출 경로가 같은 복원 몸체를 사용한다.
void ConnectPriestPanic() {
    if (!motion || !carrierTarget || priestPanic) throw std::logic_error("사제 panic 설치 순서 오류");PriestPanicHooks hooks;
    const auto truncate=[](double value) { if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("사제 panic 현재 좌표 CRT 범위");return static_cast<std::int32_t>(value); }; // 실제 지형의 정상 signed 좌표만 절삭한다.
    hooks.temple=[this](float x,float y) { return motion->PriestArrival().TempleAt(x,y); };hooks.maximum=[this](Sid sid) { return hp.MaxHitPoints(sid); };
    hooks.land=[this,truncate](float x,float y) { const auto cx=truncate(x),cy=truncate(y);if (cx<0 || cy<0 || cx>=256 || cy>=256) return false;const Sid surface{core.hash.Entries(0)[static_cast<std::size_t>(cy)*256+cx]};return surface.value && core.pool.Slot(surface)[10]==terrainType; };
    hooks.region=[this,truncate](float x,float y) { const auto snap=[truncate](float value) { return truncate(static_cast<float>(truncate(double(value)+0.99999F))); };return motion->Terrain().RegionAt(snap(x),snap(y)); }; //46f530의41d7a0 저장 경계 뒤 같은 실제 지역 getter다.
    hooks.random=[this](float low,float high) { return double(random.Next(65536))/65536.0*std::abs(double(high)-low)+low; };
    hooks.target=[this](Sid sid,Sid temple) { return carrierTarget->StartTarget(sid,temple,0,0); };hooks.point=[this](Sid sid,float x,float y) { return StartMotion({sid,{x,y},0,0,0,0,0}); };
    priestPanic=std::make_unique<RawPriestPanic>(core.pool,dudeSpeeds,std::move(hooks));
}
