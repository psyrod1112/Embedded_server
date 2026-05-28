// FCM은 Firebase 설정(google-services.json) 완료 후 활성화
class FcmService {
  static final FcmService _instance = FcmService._();
  factory FcmService() => _instance;
  FcmService._();

  Future<void> init() async {}
}
