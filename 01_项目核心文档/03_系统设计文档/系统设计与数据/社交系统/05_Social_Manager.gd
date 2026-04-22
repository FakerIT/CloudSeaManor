extends Node
class_name SocialManager

var affection_map := {}
var heart_event_state := {}
var paused_event := null

func add_affection(npc_id: String, value: int) -> void:
	if not affection_map.has(npc_id):
		affection_map[npc_id] = 0
	affection_map[npc_id] += value
	_check_heart_lock(npc_id)

func _check_heart_lock(npc_id: String) -> void:
	# Prevent heart overflow before required event is completed.
	# Example: if 2-heart event not completed, cap affection at threshold - 1.
	pass

func try_trigger_event(event_id: String) -> bool:
	if not _is_event_condition_met(event_id):
		return false
	heart_event_state[event_id] = "running"
	_start_dialogue_event(event_id)
	return true

func pause_current_event(reason: String) -> void:
	# Called when force sleep, stamina exhaustion, or scene interruption occurs.
	if paused_event == null:
		paused_event = _get_current_event_id()
	if paused_event != null:
		heart_event_state[paused_event] = "paused"

func resume_paused_event() -> bool:
	if paused_event == null:
		return false
	if not _is_event_condition_met(paused_event):
		return false
	heart_event_state[paused_event] = "running"
	_start_dialogue_event(paused_event)
	return true

func complete_event(event_id: String) -> void:
	heart_event_state[event_id] = "completed"
	if paused_event == event_id:
		paused_event = null
	_grant_event_reward(event_id)

func _is_event_condition_met(event_id: String) -> bool:
	# Read trigger table data and validate:
	# season, weather, time window, map/anchor, heart level, prerequisites.
	return true

func _start_dialogue_event(event_id: String) -> void:
	# Hook to Dialogue Manager / Ink runtime.
	pass

func _grant_event_reward(event_id: String) -> void:
	# Apply item rewards, affection bonus, unlock flags.
	pass

func _get_current_event_id() -> String:
	# Runtime event tracker entry.
	return ""
