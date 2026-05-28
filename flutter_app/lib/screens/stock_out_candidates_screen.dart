import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:intl/intl.dart';

import '../models/stock_event.dart';
import '../providers/food_provider.dart';
import '../services/api_service.dart';

class StockOutCandidatesScreen extends ConsumerWidget {
  const StockOutCandidatesScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final pendingAsync = ref.watch(pendingStockOutsProvider);

    return Scaffold(
      appBar: AppBar(title: const Text('출고 확인')),
      body: pendingAsync.when(
        data: (events) {
          if (events.isEmpty) {
            return const Center(child: Text('대기 중인 출고 이벤트 없음'));
          }
          return ListView.builder(
            itemCount: events.length,
            itemBuilder: (_, i) => _PendingEventCard(
              event: events[i],
              onConfirm: (typeId) async {
                await ApiService().confirmStockOut(events[i].eventId, typeId);
                ref.invalidate(pendingStockOutsProvider);
                ref.invalidate(foodTypesProvider);
              },
            ),
          );
        },
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('오류: $e')),
      ),
    );
  }
}

class _PendingEventCard extends StatelessWidget {
  final PendingStockOut event;
  final Future<void> Function(String typeId) onConfirm;

  const _PendingEventCard({required this.event, required this.onConfirm});

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.all(12),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              '${event.deltaGram.toStringAsFixed(0)}g 감소 감지',
              style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 16),
            ),
            Text(
              DateFormat('MM/dd HH:mm').format(event.createdAt),
              style: const TextStyle(color: Colors.grey),
            ),
            const SizedBox(height: 12),
            const Text('출고된 식품 선택:', style: TextStyle(fontWeight: FontWeight.w500)),
            const SizedBox(height: 8),
            ...event.candidates.map(
              (c) => ListTile(
                leading: const Icon(Icons.inventory_2_outlined),
                title: Text(c.typeName),
                subtitle: Text(
                  '유통기한: ${DateFormat('yyyy.MM.dd').format(c.expiryDate)}  '
                  '무게오차: ${c.weightDiffPct.toStringAsFixed(1)}%',
                ),
                trailing: ElevatedButton(
                  onPressed: () => onConfirm(c.typeId),
                  child: const Text('선택'),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
