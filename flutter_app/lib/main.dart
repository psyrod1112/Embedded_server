import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import 'providers/auth_provider.dart';
import 'screens/food_items_screen.dart';
import 'screens/home_screen.dart';
import 'screens/login_screen.dart';
import 'screens/manual_stock_screen.dart';
import 'screens/stock_out_candidates_screen.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  // Firebase는 google-services.json 설정 후 활성화
  runApp(const ProviderScope(child: FridgeFifoApp()));
}

class FridgeFifoApp extends ConsumerWidget {
  const FridgeFifoApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isLoggedIn = ref.watch(authProvider);

    final router = GoRouter(
      initialLocation: isLoggedIn ? '/' : '/login',
      redirect: (context, state) {
        if (!isLoggedIn && state.matchedLocation != '/login') return '/login';
        if (isLoggedIn && state.matchedLocation == '/login') return '/';
        return null;
      },
      routes: [
        GoRoute(path: '/login', builder: (_, __) => const LoginScreen()),
        GoRoute(
          path: '/',
          builder: (_, __) => const HomeScreen(),
        ),
        GoRoute(
          path: '/foods/:typeId',
          builder: (_, state) => FoodItemsScreen(
            typeId: state.pathParameters['typeId']!,
            typeName: state.extra as String? ?? '',
          ),
        ),
        GoRoute(
          path: '/stock-out-pending',
          builder: (_, __) => const StockOutCandidatesScreen(),
        ),
        GoRoute(
          path: '/manual-stock',
          builder: (_, __) => const ManualStockScreen(),
        ),
      ],
    );

    return MaterialApp.router(
      title: 'FridgeFIFO',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      routerConfig: router,
    );
  }
}
