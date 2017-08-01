N = 2^{K_0} -- число узлов
K = 20 > K_0 -- макс. число известных соседей
M = K/2.5 -- о каком числе лучших соседей отдавать инфу 

# Добавление нового узла

* Знать UUID точки входа
* Случай одной точки входа: первому запросу find_M_closest при добавлении нового узла отдать ровно K (ровно M?) самых хороших соседей
(даже если они хуже точки входа)?
* В последующих запросах опрашиваемый узел отдает не более $M$ вариантов, лучших чем он.
* Когда список $K$-лучших не меняется 

#### Возможные улучшения
* __Решение проблемы неизвестного новичка?__ Если find_M_closest(target_UUID) вызван в процессе добавления нового узла,
то можно заменить одного из соседей на target_UUID. Чтобы нельзя было "отжать" у старого узла знания о всех соседях, введём лимит на такую замену. 

# Ребаланс при штатном удалении

* Перед удалением разослать $K$ самым близким узлам (увы, возможно им этот узел не близок) список соседей удаляемого узла.

#### Возможные улучшения или не очень
* Узел хранит инфу о том, кто ЕГО знает и использует её только для ребаланса при удалении (не пингует/очень редко пингует). При удалении
тем, кто ещё жив, разсылает данные для ребаланса.
* Так ли много узлов, для которых удаляемый входит в $K$ ближайщих? Скажем, при N = 2^10. 

# Пинг K-известных узлов
* После чего считать, что известный узел упал? 

# Если обнаружено нештатное падение соседа
* Запросить у соседей списки их соседей. Если так будут найдены новые соседи, то добавить на место упавшего лучших из них.

# Поиск (или добавление) записи в Distributed UUID Table (Спизженно с habrahabr.ru/post/325528/ и немного додумано)
1) У "уже известных" узлов берутся соседи, которые ближе, чем "уже известные". (В каждый момент храним K лучших?)
2) Повторяем 1. пока есть узлы лучше (find_M_closest).
3) Узлы с distance[target_UUID, node_UUID] < tolerance опрашиваются на наличие в target_UUID в их таблицах.
* Голосование при несовпадении ответов?

# Удаление владельцем, инвалидация записей в Distributed UUID Table
* С каждой записью хранить публичный ключ владельца
* Как понять, что запись инвалидировалась?

