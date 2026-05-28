#include "HX711.h"
#include <stdio.h>   // C 언어 표준 입출력 (printf)
#include <unistd.h>  // C 언어 표준 시간 지연 (sleep, usleep)

int main() {
    // 1. 무게센서 객체 생성
    HX711 scale;

    // 2. 사용할 라즈베리 파이 BCM GPIO 핀 번호 설정
    // 예시: DT 핀 = GPIO 5, SCK 핀 = GPIO 6
    unsigned char pin_DT = 5;
    unsigned char pin_SCK = 6;

    printf("무게센서 초기화 중... (DT: %d, SCK: %d)\n", pin_DT, pin_SCK);

    // 3. 라이브러리 시작
    // 세 번째 인자(fastProcessor)를 true로 주어 라즈베리파이 타이밍 딜레이를 켭니다.
    scale.begin(pin_DT, pin_SCK, true);

    // 4. 영점 조절 (Tare)
    // 센서 위에 아무것도 올려두지 않은 상태여야 합니다.
    printf("영점 조절(Tare) 중입니다. 센서 위를 비워주세요...\n");
    sleep(1); // C 언어 스타일의 1초 대기
    
    scale.tare(10); // 10번 읽어서 평균값으로 영점 잡기
    printf("영점 조절 완료! 현재 오프셋 값: %d\n", scale.get_offset());

    // 5. 보정 계수(Scale) 설정 (우선 기본값 1.0)
    scale.set_scale(1.0); 

    // 6. 중간값(Median) 필터 적용 (노이즈 튀는 현상 방지)
    scale.set_median_mode();

    printf("\n=== 무게 측정을 시작합니다. (종료: Ctrl + C) ===\n");

    // 7. 무한 루프 돌며 무게 출력
    while (1) {
        // 5번 측정해서 필터링된 최종 무게를 가져옴
        float weight = scale.get_units(5);
        
        // %.2f를 이용해 소수점 둘째 자리까지만 깔끔하게 출력
        printf("측정된 무게: %.2f\n", weight);

        // 0.5초(500,000 마이크로초) 주기로 루프 돌리기
        usleep(500000); 
    }

    return 0;
}
