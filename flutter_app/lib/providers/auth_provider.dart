import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../services/auth_service.dart';

final authProvider = StateNotifierProvider<AuthNotifier, bool>((ref) {
  return AuthNotifier();
});

class AuthNotifier extends StateNotifier<bool> {
  AuthNotifier() : super(false) {
    _check();
  }

  Future<void> _check() async {
    state = await AuthService().isLoggedIn();
  }

  Future<void> login(String username, String password) async {
    await AuthService().login(username, password);
    state = true;
  }

  Future<void> logout() async {
    await AuthService().logout();
    state = false;
  }
}
