#include "HX711.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 1. HX711 객체 생성
    HX711 scale;

    // 2. 사용할 라즈베리 파이 BCM GPIO 핀 번호 설정
    // 예시: DT 핀 = GPIO 5, SCK 핀 = GPIO 6
    uint8_t pin_DT = 5;
    uint8_t pin_SCK = 6;

    std::cout << "무게센서 초기화 중... (DT: " << (int)pin_DT 
              << ", SCK: " << (int)pin_SCK << ")" << std::endl;

    // 3. 라이브러리 시작 (핀 등록 및 하드웨어 준비)
    // 세 번째 인자(fastProcessor): 라즈베리 파이는 프로세서가 빠르므로 true로 설정해 타이밍 마이크로 딜레이를 줍니다.
    scale.begin(pin_DT, pin_SCK, true);

    // 4. 영점 조절 (Tare)
    // 센서 위에 아무것도 올려두지 않은 상태에서 실행해야 합니다.
    std::cout << "영점 조절(Tare) 중입니다. 센서 위를 비워주세요..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 안정화를 위한 잠시 대기
    scale.tare(10); // 10번 읽어서 평균값으로 영점 잡기
    std::cout << "영점 조절 완료! 현재 오프셋 값: " << scale.get_offset() << std::endl;

    // 5. 보정 계수(Scale) 설정
    // 아직 정확한 계수를 모른다면 기본값인 1.0으로 두고 테스트합니다.
    // 나중에 100g짜리 물건을 올렸는데 만약 get_value() 값이 50000이 나오면,
    // scale.set_scale(50000.0 / 100.0); 즉, scale.set_scale(500.0); 으로 고치면 g 단위가 됩니다.
    scale.set_scale(1.0); 

    // 6. 데이터 필터링 모드 설정 (선택 사항)
    // Rob Tillaart 라이브러리의 장점인 '중간값(Median) 필터'를 적용해 튐 현상을 잡습니다.
    scale.set_median_mode();

    std::cout << "\n=== 무게 측정을 시작합니다. (종료: Ctrl + C) ===" << std::endl;

    // 7. 무한 루프 돌며 무게 출력
    while (true) {
        // get_units(5): 5번 측정해서 필터링된 최종 무게(보정계수가 반영된 값)
        float weight = scale.get_units(5);
        
        // 원시 가공 데이터(오프셋만 뺀 값)를 보고 싶다면 아래 주석을 해제하세요.
        // float raw_value = scale.get_value(5);

        std::cout << "측정된 무게: " << weight << std::endl;

        // 500ms(0.5초) 주기로 화면에 출력
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
