import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../providers/auth_provider.dart';
import '../providers/food_provider.dart';
import '../widgets/food_type_card.dart';

class HomeScreen extends ConsumerWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final typesAsync = ref.watch(foodTypesProvider);
    final pendingAsync = ref.watch(pendingStockOutsProvider);

    return Scaffold(
      appBar: AppBar(
        title: const Text('FridgeFIFO'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () {
              ref.invalidate(foodTypesProvider);
              ref.invalidate(pendingStockOutsProvider);
            },
          ),
          IconButton(
            icon: const Icon(Icons.logout),
            onPressed: () => ref.read(authProvider.notifier).logout(),
          ),
        ],
      ),
      body: Column(
        children: [
          // ── Pending stock-out banner ────────────────────────────
          pendingAsync.when(
            data: (pending) {
              if (pending.isEmpty) return const SizedBox.shrink();
              return MaterialBanner(
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                content: Text('출고 확인 필요 ${pending.length}건'),
                backgroundColor: Colors.orange.shade100,
                actions: [
                  TextButton(
                    onPressed: () => context.push('/stock-out-pending'),
                    child: const Text('확인'),
                  ),
                ],
              );
            },
            loading: () => const SizedBox.shrink(),
            error: (_, __) => const SizedBox.shrink(),
          ),

          // ── Food type list ──────────────────────────────────────
          Expanded(
            child: typesAsync.when(
              data: (types) {
                if (types.isEmpty) {
                  return const Center(child: Text('등록된 식품이 없습니다.\n입고 후 자동으로 추가됩니다.', textAlign: TextAlign.center));
                }
                return ListView.builder(
                  padding: const EdgeInsets.all(12),
                  itemCount: types.length,
                  itemBuilder: (_, i) => FoodTypeCard(
                    foodType: types[i],
                    onTap: () => context.push('/foods/${types[i].typeId}', extra: types[i].typeName),
                  ),
                );
              },
              loading: () => const Center(child: CircularProgressIndicator()),
              error: (e, _) => Center(child: Text('오류: $e')),
            ),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        icon: const Icon(Icons.add),
        label: const Text('수동 입고'),
        onPressed: () => context.push('/manual-stock'),
      ),
    );
  }
}
