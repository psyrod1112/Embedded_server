import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:intl/intl.dart';

import '../models/food_item.dart';
import '../providers/food_provider.dart';
import '../services/api_service.dart';
import '../widgets/food_item_tile.dart';

class FoodItemsScreen extends ConsumerStatefulWidget {
  final String typeId;
  final String typeName;

  const FoodItemsScreen({
    super.key,
    required this.typeId,
    required this.typeName,
  });

  @override
  ConsumerState<FoodItemsScreen> createState() => _FoodItemsScreenState();
}

class _FoodItemsScreenState extends ConsumerState<FoodItemsScreen> {
  Future<void> _renameType() async {
    final ctrl = TextEditingController(text: widget.typeName);
    final newName = await showDialog<String>(
      context: context,
      builder: (_) => AlertDialog(
        title: const Text('타입 이름 변경'),
        content: TextField(controller: ctrl, decoration: const InputDecoration(labelText: '새 이름')),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('취소')),
          TextButton(onPressed: () => Navigator.pop(context, ctrl.text), child: const Text('저장')),
        ],
      ),
    );
    if (newName == null || newName.isEmpty) return;
    await ApiService().renameType(widget.typeId, newName);
    ref.invalidate(foodTypesProvider);
    if (mounted) ref.invalidate(foodItemsProvider(widget.typeId));
  }

  Future<void> _stockOut(FoodItem item) async {
    final confirm = await showDialog<bool>(
      context: context,
      builder: (_) => AlertDialog(
        title: const Text('출고 확인'),
        content: Text('${item.typeName}\n유통기한: ${DateFormat('yyyy.MM.dd').format(item.expiryDate)}\n1개 출고하시겠습니까?'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context, false), child: const Text('취소')),
          TextButton(onPressed: () => Navigator.pop(context, true), child: const Text('출고')),
        ],
      ),
    );
    if (confirm != true) return;
    await ApiService().manualStockOut(item.itemId);
    if (mounted) ref.invalidate(foodItemsProvider(widget.typeId));
  }

  Future<void> _restore(FoodItem item) async {
    await ApiService().restoreItem(item.itemId);
    ref.invalidate(foodItemsProvider(widget.typeId));
  }

  @override
  Widget build(BuildContext context) {
    final itemsAsync = ref.watch(foodItemsProvider(widget.typeId));

    return Scaffold(
      appBar: AppBar(
        title: Text(widget.typeName),
        actions: [
          IconButton(
            icon: const Icon(Icons.edit),
            onPressed: _renameType,
          ),
        ],
      ),
      body: itemsAsync.when(
        data: (items) {
          if (items.isEmpty) {
            return const Center(child: Text('재고 없음'));
          }
          return ListView.builder(
            itemCount: items.length,
            itemBuilder: (_, i) => FoodItemTile(
              item: items[i],
              onStockOut: () => _stockOut(items[i]),
              onRestore: () => _restore(items[i]),
            ),
          );
        },
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('오류: $e')),
      ),
    );
  }
}
