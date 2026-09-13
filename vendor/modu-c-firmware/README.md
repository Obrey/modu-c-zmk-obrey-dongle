> [!NOTE]
> 키맵 편집 및 다운로드: https://github.com/22sh22/modu-c-zmk-config

# MODU Firmware

MODU-C 키보드용 ZMK 펌웨어 소스입니다.

빌드하려면 `west build`가 동작하는 ZMK 개발 환경과 Zephyr SDK가 필요합니다.

Windows에서는 다음 명령으로 좌·우 BLE 펌웨어를 생성할 수 있습니다.

```bat
build.bat C:\zmk\app
```

생성된 `modu_left.uf2`와 `modu_right.uf2`는 `outputs` 폴더에 저장됩니다.

**키매핑 파일의 위치는 `modu-module/boards/shields/modu/modu.keymap` 입니다.**

미리 빌드된 펌웨어는 GitHub Releases에서 배포합니다.

## License

Copyright (c) 2026 EKS Inc. · Created by Ryu

MODU 고유 소스는 비상업적 사용·수정·동일 조건 재배포만 허용됩니다.
자세한 조건은 [LICENSE](LICENSE), 외부 코드 고지는 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)를 참고하세요.
