import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:intl/intl.dart';

import '../models/food_type.dart';
import '../providers/food_provider.dart';
import '../services/api_service.dart';

class ManualStockScreen extends ConsumerStatefulWidget {
  const ManualStockScreen({super.key});

  @override
  ConsumerState<ManualStockScreen> createState() => _ManualStockScreenState();
}

class _ManualStockScreenState extends ConsumerState<ManualStockScreen>
    with SingleTickerProviderStateMixin {
  late final TabController _tab;

  @override
  void initState() {
    super.initState();
    _tab = TabController(length: 2, vsync: this);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('수동 입/출고'),
        bottom: TabBar(
          controller: _tab,
          tabs: const [Tab(text: '수동 입고'), Tab(text: '수동 출고')],
        ),
      ),
      body: TabBarView(
        controller: _tab,
        children: const [_ManualInTab(), _ManualOutTab()],
      ),
    );
  }
}

// ── Manual Stock-In ────────────────────────────────────────────────────────────

class _ManualInTab extends ConsumerStatefulWidget {
  const _ManualInTab();

  @override
  ConsumerState<_ManualInTab> createState() => _ManualInTabState();
}

class _ManualInTabState extends ConsumerState<_ManualInTab> {
  FoodType? _selectedType;
  DateTime? _expiryDate;
  final _qtyCtrl = TextEditingController(text: '1');
  final _weightCtrl = TextEditingController();
  bool _loading = false;

  Future<void> _createNewType() async {
    final nameCtrl = TextEditingController();
    final weightCtrl = TextEditingController();
    final result = await showDialog<bool>(
      context: context,
      builder: (_) => AlertDialog(
        title: const Text('새 식품 타입 추가'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: nameCtrl,
              decoration: const InputDecoration(labelText: '타입 이름 (예: 바나나우유)'),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: weightCtrl,
              keyboardType: TextInputType.number,
              decoration: const InputDecoration(labelText: '단위 무게 (g)'),
            ),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context, false), child: const Text('취소')),
          TextButton(onPressed: () => Navigator.pop(context, true), child: const Text('추가')),
        ],
      ),
    );
    if (result != true || nameCtrl.text.isEmpty || weightCtrl.text.isEmpty) return;
    try {
      final newType = await ApiService().createType(
        nameCtrl.text.trim(),
        double.parse(weightCtrl.text),
      );
      ref.invalidate(foodTypesProvider);
      setState(() => _selectedType = newType);
    } catch (e) {
      if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('오류: $e')));
    }
  }

  Future<void> _pickDate() async {
    final picked = await showDatePicker(
      context: context,
      initialDate: DateTime.now().add(const Duration(days: 30)),
      firstDate: DateTime.now(),
      lastDate: DateTime.now().add(const Duration(days: 730)),
    );
    if (picked != null) setState(() => _expiryDate = picked);
  }

  Future<void> _submit() async {
    if (_selectedType == null || _expiryDate == null || _weightCtrl.text.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('모든 항목을 입력하세요')));
      return;
    }
    setState(() => _loading = true);
    try {
      await ApiService().manualStockIn(
        typeId: _selectedType!.typeId,
        expiryDate: _expiryDate!,
        quantity: int.tryParse(_qtyCtrl.text) ?? 1,
        weightGram: double.tryParse(_weightCtrl.text) ?? 0,
      );
      ref.invalidate(foodTypesProvider);
      if (mounted) Navigator.pop(context);
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('오류: $e')));
    } finally {
      setState(() => _loading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final typesAsync = ref.watch(foodTypesProvider);
    return Padding(
      padding: const EdgeInsets.all(20),
      child: Column(
        children: [
          Row(
            children: [
              Expanded(
                child: typesAsync.when(
                  data: (types) => DropdownButtonFormField<FoodType>(
                    decoration: const InputDecoration(labelText: '식품 타입', border: OutlineInputBorder()),
                    value: types.any((t) => t.typeId == _selectedType?.typeId) ? _selectedType : null,
                    items: types.map((t) => DropdownMenuItem(value: t, child: Text(t.typeName))).toList(),
                    onChanged: (t) => setState(() => _selectedType = t),
                  ),
                  loading: () => const CircularProgressIndicator(),
                  error: (_, __) => const Text('타입 로드 실패'),
                ),
              ),
              const SizedBox(width: 8),
              IconButton.filled(
                icon: const Icon(Icons.add),
                tooltip: '새 타입 추가',
                onPressed: _createNewType,
              ),
            ],
          ),
          const SizedBox(height: 16),
          InkWell(
            onTap: _pickDate,
            child: InputDecorator(
              decoration: const InputDecoration(labelText: '유통기한', border: OutlineInputBorder()),
              child: Text(
                _expiryDate == null ? '날짜 선택' : DateFormat('yyyy.MM.dd').format(_expiryDate!),
              ),
            ),
          ),
          const SizedBox(height: 16),
          TextField(
            controller: _qtyCtrl,
            keyboardType: TextInputType.number,
            decoration: const InputDecoration(labelText: '수량', border: OutlineInputBorder()),
          ),
          const SizedBox(height: 16),
          TextField(
            controller: _weightCtrl,
            keyboardType: TextInputType.number,
            decoration: const InputDecoration(labelText: '단위 무게 (g)', border: OutlineInputBorder()),
          ),
          const SizedBox(height: 24),
          SizedBox(
            width: double.infinity,
            child: ElevatedButton(
              onPressed: _loading ? null : _submit,
              child: _loading ? const CircularProgressIndicator() : const Text('입고'),
            ),
          ),
        ],
      ),
    );
  }
}

// ── Manual Stock-Out ───────────────────────────────────────────────────────────

class _ManualOutTab extends ConsumerWidget {
  const _ManualOutTab();

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final typesAsync = ref.watch(foodTypesProvider);
    return typesAsync.when(
      data: (types) => ListView.builder(
        padding: const EdgeInsets.all(12),
        itemCount: types.length,
        itemBuilder: (_, i) {
          final t = types[i];
          return ListTile(
            title: Text(t.typeName),
            subtitle: Text('재고: ${t.totalQuantity}개  |  단위무게: ${t.unitWeightGram.toStringAsFixed(0)}g'),
            trailing: const Icon(Icons.chevron_right),
            onTap: () => Navigator.of(context).push(
              MaterialPageRoute(
                builder: (_) => _TypeOutScreen(typeId: t.typeId, typeName: t.typeName),
              ),
            ),
          );
        },
      ),
      loading: () => const Center(child: CircularProgressIndicator()),
      error: (e, _) => Center(child: Text('오류: $e')),
    );
  }
}

class _TypeOutScreen extends ConsumerWidget {
  final String typeId;
  final String typeName;
  const _TypeOutScreen({required this.typeId, required this.typeName});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final itemsAsync = ref.watch(foodItemsProvider(typeId));
    return Scaffold(
      appBar: AppBar(title: Text(typeName)),
      body: itemsAsync.when(
        data: (items) => ListView.builder(
          itemCount: items.length,
          itemBuilder: (_, i) {
            final item = items[i];
            return ListTile(
              leading: CircleAvatar(child: Text('${item.fifoPosition}')),
              title: Text('유통기한: ${DateFormat('yyyy.MM.dd').format(item.expiryDate)}'),
              subtitle: Text('수량: ${item.quantity}개'),
              trailing: TextButton(
                onPressed: () async {
                  await ApiService().manualStockOut(item.itemId);
                  ref.invalidate(foodItemsProvider(typeId));
                },
                child: const Text('출고'),
              ),
            );
          },
        ),
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('오류: $e')),
      ),
    );
  }
}
