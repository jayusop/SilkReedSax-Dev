# SilkReed Sax 배포 가이드 (내부용)

배포 저장소는 `~/Develop/SAX-VST-doc` 하나이며 `mixer-doc`(Audio-Mixer)과 같은 구조입니다.

```
SAX-VST-doc/   →  github.com/jayusop/SilkReed-Sax  (Public)
├── README.md, README.en.md, README.en-GB.md, README.zh-CN.md, README.ja.md, README.fr.md
├── LICENSE            AGPLv3 원문
├── LICENSE.ko.md      한국어 안내
└── docs/              GitHub Pages (main / docs)  →  https://jayusop.github.io/SilkReed-Sax/
    ├── index.html, licenses.html, THIRD_PARTY_NOTICES.txt, .nojekyll
    └── assets/ (config.js, script.js, style.css, icon.png, screenshot.png)
```

이 소스 폴더(`SAX-VST`)는 GitHub에 올리지 않습니다. AGPLv3 조건을 지키기 위해 **각 릴리스에 소스 zip을 함께 첨부**합니다.

## 1. 배포 저장소 올리기 (처음 한 번)

GitHub에서 빈 Public 저장소 `SilkReed-Sax`를 만든 뒤:

```sh
cd ~/Develop/SAX-VST-doc
git init -b main && git add . && git commit -m "SilkReed Sax 1.0.0"
git remote add origin https://github.com/jayusop/SilkReed-Sax.git
git push -u origin main
```

Settings → Pages → Source: *Deploy from a branch*, Branch: `main` / `/docs` → Save.
About(톱니바퀴) → Website에 `https://jayusop.github.io/SilkReed-Sax/`를 넣어 두세요.

## 0. 처음 배포 전에 (한 번만)

- README의 「배포 전에 결정해야 할 것」 중 **1번(AGPLv3 오픈소스)** 으로 배포합니다. 소스 파일 상단의 "All rights reserved." 문구를 AGPLv3 표기로 바꾸고, README의 저작권 문단도 맞춰 주세요. `LICENSE`는 아래 소스 zip을 만들 때 자동으로 들어갑니다.
- 제3자 고지는 `THIRD_PARTY_NOTICES.txt` 하나로 관리합니다(JUCE를 바꾸면 `python3 Tools/make_notices.py`). 다시 만들었다면 소개 페이지에도 복사하세요:
  `cp THIRD_PARTY_NOTICES.txt ../SAX-VST-doc/docs/`

## 2. 릴리스 만들기 (버전마다)

```sh
cd ~/Develop/SAX-VST
V=1.0.0
./build.sh universal                         # 유니버설 Release 빌드 + CPU 검사 + ad-hoc 서명 → build-universal/

# 플러그인 zip
ART=build-universal/SilkReedSax_artefacts/Release
rm -rf dist && mkdir -p "dist/SilkReed Sax"
cp -R "$ART/VST3/SilkReed Sax.vst3" "$ART/AU/SilkReed Sax.component" "$ART/Standalone/SilkReed Sax.app" \
      ../SAX-VST-doc/LICENSE THIRD_PARTY_NOTICES.txt "dist/SilkReed Sax/"
(cd dist && ditto -c -k --keepParent "SilkReed Sax" "SilkReed-Sax-$V-macOS.zip")

# 소스 zip (빌드 산출물·JUCE·개인 설정 제외, LICENSE 포함)
mkdir -p dist/SilkReed-Sax-source
rsync -a --exclude 'build' --exclude 'build-*' --exclude ThirdParty/JUCE --exclude .claude \
      --exclude dist --exclude .DS_Store --exclude RELEASE.md ./ dist/SilkReed-Sax-source/
cp ../SAX-VST-doc/LICENSE dist/SilkReed-Sax-source/
(cd dist && ditto -c -k --keepParent SilkReed-Sax-source "SilkReed-Sax-$V-source.zip")
```

GitHub → Releases → Draft a new release → 태그 `v1.0.0` → 아래 두 파일 첨부 → Publish.

| 첨부 파일 | 내용 |
|---|---|
| `SilkReed-Sax-1.0.0-macOS.zip` | VST3 / AU / Standalone (유니버설, macOS 12+) + 라이선스 |
| `SilkReed-Sax-1.0.0-source.zip` | 전체 소스 (AGPLv3 소스 제공용) |

소개 페이지의 다운로드 버튼은 이름에 `source`가 없는 zip으로 자동 연결됩니다.

## 3. 화면이 바뀌었을 때

```sh
build-universal/SilkReedTool_artefacts/Release/SilkReedTool snapshot ../SAX-VST-doc/docs/assets/screenshot.png
```

로컬 미리보기: `cd ~/Develop/SAX-VST-doc/docs && python3 -m http.server 8000` → http://localhost:8000
