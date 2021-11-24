# 전자산업기사 실기 풀이
- 비번호는 선착순으로 정함.
- 수험번호 적을 일 있으니 알려달라고 하면 알려줌.   

<br />
<br />


## 과제1 [회로 스케치]
- 회로 도면 보고 빈칸에 들어갈 부품을 찾아 회로를 완성하는 것이 문제.
- 회로 중간에 소자 한 개 or 두 개를 빈칸 처리함.
- 문제에서 회로 도면이 주어짐. 부품 이름 다 정해져 있음.
- 라이브러리 따로 만들 일 없음.
- 회로 도면 빈칸에 있는 빈 부품이 뭔지 찾고, Top, Bottom 배선도에서 그 부품의 주변 연결 상태를 보고 빈칸을 채워야함.
- 회로 완성 후 프로젝트 파일(.opj)을 감독관 USB에 옮기고, 다른 pc에서 프린트 함.
- 빈칸 외에 회로에서 점 하나를 안 찍거나, 다른 부품을 배치하는 등 기존 회로와 틀리면 실격 처리됨.
- 회로도는 [Schematic.pdf]를 참고.   

<br />
<br />


## 과제2 [회로 및 펌웨어 설계]
- LCD 뒤에 저항 값을 조절할 작은 드라이버 필요함. 안 그러면 샤프심으로 돌려야 됨..ㅎ
- 본체가 책상 밑에 있을 수 있기 때문에 아두이노 연결 USB선이 긴 걸로 챙기면 좋음.
- 아두이노 호환 보드 사용 가능.
- 아두이노 메가 보드, 점퍼선, 브래드 보드(2000홀 이상) 필수 지참.
- 아두이노 회로도는 [Arduino_Schematic.png]을 참고.   


<br />
<br />

### 만든이 : 동서울대학교 전자공학과 MBS&YHA
- 완벽한 코드는 아니지만 완벽하게 동작합니다.
- 아두이노 스위치 Pin을 변경할 시 인터럽트가 가능한 Pin으로 설정하십시오.
- 아두이노 Trig Pin과 Echo Pin은 반드시 PWM이 출력 가능한 Pin으로 설정하십시오.
- ORCAD Lite 버전은 [https://drive.google.com/file/d/16XdJK0gyOi9nfx-gus6dM4nz-UIs_kke/view?usp=sharing] 에서 다운 가능합니다.
- ORCAD Lite 버전 라이브러리에는 Connector가 없으니 [Connector.OLB]를 검색해 다운 받아 사용하십시오.
- ORCAD 사용법은 유튜브에서 숙달 하십시오.
