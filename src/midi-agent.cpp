/*
obs-midi

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "midi-agent.h"
#include "config.h"
#include "device-manager.h"
#include "obs-midi.h"
#include "utils.h"
#include <QDebug>
#include <QtCore/QTime>
#include <functional>
#include <map>
#include <obs-frontend-api.h>
#include <string>

///////////////////////
/* MIDI HOOK ROUTES */
//////////////////////
/*
// BUTTON ACTIONS
QMap<QString, std::function<void(MidiHook *hook, int midiVal)>> funcMap = {
    {"Set Current Scene",
     [](MidiHook *hook, int midiVal) {
       OBSController::SetCurrentScene(hook->param1.toStdString().c_str());
     }},
    {"Set Preview Scene",
     [](MidiHook *hook, int midiVal) {
       OBSController::SetPreviewScene(hook->param1.toStdString().c_str());
     }},
    {"Set Current Scene Collection",
     [](MidiHook *hook, int midiVal) {
       OBSController::SetCurrentSceneCollection(
           hook->param1.toStdString().c_str());
     }},
    {"Reset Scene Item",
     [](MidiHook *hook, int midiVal) {
       OBSController::ResetSceneItem(hook->param1.toStdString().c_str(),
                                     hook->param2.toStdString().c_str());
     }},
    {"TransitionToProgram",
     [](MidiHook *hook, int midiVal) {

     }},
    {"Set Current Transition",
     [](MidiHook *hook, int midiVal) {
                 OBSController::SetCurrentTransition(hook->param1);
     }},
    {"SetTransitionDuration",
     [](MidiHook *hook, int midiVal) {
       if (Utils::is_number(hook->param1)) {
         OBSController::SetTransitionDuration(hook->param1.toInt());
       } else {
         OBSController::SetTransitionDuration(midiVal);
       }
     }},
    {"Toggle Mute",
     [](MidiHook *hook, int midiVal) { OBSController::ToggleMute(hook->param1);
}},
    {"Set Mute", [](MidiHook *hook,
                    int midiVal) { OBSController::SetMute(hook->param1,
midiVal); }},
    {"Start Stop Streaming",
     [](MidiHook *hook, int midiVal) { OBSController::StartStopStreaming(); }},
    {"Start Streaming",
     [](MidiHook *hook, int midiVal) { OBSController::StartStreaming(); }},
    {"Stop Streaming",
     [](MidiHook *hook, int midiVal) { OBSController::StopStreaming(); }},
    {"Start Stop Recording",
     [](MidiHook *hook, int midiVal) { OBSController::StartStopRecording(); }},
    {"Start Recording",
     [](MidiHook *hook, int midiVal) { OBSController::StartRecording(); }},
    {"Stop Recording",
     [](MidiHook *hook, int midiVal) { OBSController::StopRecording(); }},
    {"Pause Recording",
     [](MidiHook *hook, int midiVal) { OBSController::PauseRecording(); }},
    {"Resume Recording",
     [](MidiHook *hook, int midiVal) { OBSController::ResumeRecording(); }},
    {"Start Stop Replay Buffer",
     [](MidiHook *hook, int midiVal) {
       OBSController::StartStopReplayBuffer();
     }},
    {"Start Replay Buffer",
     [](MidiHook *hook, int midiVal) { OBSController::StartReplayBuffer(); }},
    {"Stop Replay Buffer",
     [](MidiHook *hook, int midiVal) { OBSController::StopReplayBuffer(); }},
    {"Save Replay Buffer",
     [](MidiHook *hook, int midiVal) { OBSController::SaveReplayBuffer(); }},
    {"Set Current Profile",
     [](MidiHook *hook, int midiVal) {
                 OBSController::SetCurrentProfile(hook->param1);
     }},
    {"Set Text GDIPlus Text",
     [](MidiHook *hook, int midiVal) { OBSController::SetTextGDIPlusText(); }},
    {"Set Browser Source URL",
     [](MidiHook *hook, int midiVal) { OBSController::SetBrowserSourceURL(); }},
    {"Reload Browser Source",
     [](MidiHook *hook, int midiVal) { OBSController::ReloadBrowserSource(); }},
    {"Take Source Screenshot",
     [](MidiHook *hook, int midiVal) {
                 OBSController::TakeSourceScreenshot(hook->param1);
     }},
    {"Enable Source Filter",
     [](MidiHook *hook, int midiVal) { OBSController::EnableSourceFilter(); }},
    {"Disable Source Filter",
     [](MidiHook *hook, int midiVal) { OBSController::DisableSourceFilter(); }},
    {"Toggle Source Filter",
     [](MidiHook *hook, int midiVal) { OBSController::ToggleSourceFilter(); }},
    // CC ACTIONS
    {"Set Volume",
     [](MidiHook *hook, int midiVal) {
                 OBSController::SetVolume(hook->param1,
                                          pow(Utils::mapper(midiVal), 3.0));
     }},
    {"Set Sync Offset",
     [](MidiHook *hook, int midiVal) {
                 OBSController::SetSyncOffset(
                         hook->param1,
                         (int64_t)midiVal);
     }},
    {"Set Source Position",
     [](MidiHook *hook, int midiVal) { OBSController::SetSourcePosition(); }},
    {"Set Source Rotation",
     [](MidiHook *hook, int midiVal) { OBSController::SetSourceRotation(); }},
    {"Set Source Scale",
     [](MidiHook *hook, int midiVal) { OBSController::SetSourceScale(); }},
    {"Set Gain Filter",
     [](MidiHook *hook, int midiVal) { OBSController::SetGainFilter(); }},
    {"Set Opacity",
     [](MidiHook *hook, int midiVal) { OBSController::SetOpacity(); }}};
     */
////////////////
// MIDI AGENT //
////////////////
MidiAgent::MidiAgent() {

  name = "Midi Device (uninit)";
  outname = "Midi Out Device (uninit)";
  midiin = new rtmidi::midi_in();
  midiout = new rtmidi::midi_out();
  midiin->set_callback(
      [this](const auto &message) { HandleInput(message, this); });
}

MidiAgent::~MidiAgent() {
  ClosePort();
  delete midiin;
  delete midiout;
}

/* Loads information from OBS data. (recalled from Config)
 * This will not enable the MidiAgent or open the port. (and shouldn't)
 */
void MidiAgent::Load(obs_data_t *data) {
  name = obs_data_get_string(data, "name");
  outname = obs_data_get_string(data, "outname");
  enabled = obs_data_get_bool(data, "enabled");
  bidirectional = obs_data_get_bool(data, "bidirectional");

  obs_data_array_t *hooksData = obs_data_get_array(data, "hooks");
  size_t hooksCount = obs_data_array_count(hooksData);
  for (size_t i = 0; i < hooksCount; i++) {
    obs_data_t *hookData = obs_data_array_item(hooksData, i);
    MidiHook *mh = new MidiHook(obs_data_get_string(hookData, "type"),
                                obs_data_get_int(hookData, "index"),
                                obs_data_get_int(hookData, "mchan"),
                                obs_data_get_bool(hookData, "bidirectional"),
                                obs_data_get_string(hookData, "command"),
                                obs_data_get_string(hookData, "param1"),
                                obs_data_get_string(hookData, "param2"),
                                obs_data_get_string(hookData, "param3"),
                                obs_data_get_string(hookData, "action"));
    AddMidiHook(mh);
  }
}
void MidiAgent::SendMessage(QString name, QString mType, int mIndex,
                            int channel) {

  emit this->SendNewUnknownMessage(name, mType, mIndex, channel);
}

/* Will open the port and enable this MidiAgent
 */
void MidiAgent::OpenPort(int inport) {
  try {

    midiin->open_port(inport);

    name = QString::fromStdString(midiin->get_port_name(inport));
    enabled = true;
    connected = true;
    qDebug() << "MIDI device connected In: " << inport << name;

  } catch (const rtmidi::midi_exception &error) {

    blog(1, "unable to open port %i -- &s", inport, error.what());
  }
}
void MidiAgent::OpenOutPort(int outport) {

  try {
    midiout->open_port(outport);
  } catch (const rtmidi::midi_exception &error) {

    blog(1, " Opening out port -- %i -- exception -- %s ", outport,
         error.what());
  }
  outname = QString::fromStdString(midiout->get_port_name(outport));
  enabled = true;
  connected = true;

  qDebug() << "MIDI device connected Out: " << outport << outname;
}

/* Will close the port and disable this MidiAgent
 */
void MidiAgent::ClosePort() {
  midiin->close_port();
  midiout->close_port();
  enabled = false;
  connected = false;
}

QString MidiAgent::GetName() { return name; }
QString MidiAgent::GetOutName() { return outname; }
void MidiAgent::SetOutName(QString oname) {
  if (outname != oname) {
    midiout->close_port();
    OpenOutPort(GetDeviceManager()->GetOutPortNumberByDeviceName(oname));
  }
  outname = oname;
}
bool MidiAgent::setBidirectional(bool state) {
  bidirectional = state;
  if (!state) {
    midiout->close_port();

  } else {
    if (midiout->is_port_open()) {
      midiout->close_port();
    }
    OpenOutPort(GetDeviceManager()->GetOutPortNumberByDeviceName(outname));
  }
  return state;
}

int MidiAgent::GetPort() { return port; }
bool MidiAgent::isEnabled() { return enabled; }
bool MidiAgent::isBidirectional() { return bidirectional; }
bool MidiAgent::isConnected() { return connected; }

/* Midi input callback.
 * Extend input handling functionality here.
 * For OBS command triggers, edit the funcMap instead.
 */
void MidiAgent::HandleInput(const rtmidi::message &message, void *userData) {
  MidiAgent *self = static_cast<MidiAgent *>(userData);
  if (self->enabled == false || self->connected == false) {
    return;
  }

  /*************Get Message parts***********/
  auto mType = Utils::mtype_to_string(message.get_message_type());
  int mchannel = message.get_channel();
  auto byt = message.bytes;
  auto norc = Utils::get_midi_note_or_control(message);
  auto value = Utils::get_midi_value(message);
  self->sending = true;
  /***** Send Messages to emit function *****/
  self->SendMessage(self->name, mType, norc, mchannel);

  /** check if hook exists for this note or cc index and launch it **/
  // Eventually add channel to this check.

  for (unsigned i = 0; i < self->midiHooks.size(); i++) {
    if (self->midiHooks.at(i)->type == mType &&
        self->midiHooks.at(i)->index == norc &&
        self->midiHooks.at(i)->mchan == mchannel) {
      self->TriggerInputCommand(self->midiHooks.at(i), value);
    }
  }
}

/* Triggers funcMap function. Called from HandleInput callback
 */
void MidiAgent::TriggerInputCommand(MidiHook *hook, int midiVal) {
  qDebug() << "Triggered: %d [%d] %s %s" << hook->index << midiVal
           << hook->command << hook->param1;
  // funcMap[hook->command](hook, midiVal);
}

/* Get the midi hooks for this device
 */
QVector<MidiHook *> MidiAgent::GetMidiHooks() { return midiHooks; }

/* Add a new MidiHook
 */
void MidiAgent::AddMidiHook(MidiHook *hook) {

  // connect()
  midiHooks.push_back(hook);
}

/* Remove a MidiHook
 */
void MidiAgent::RemoveMidiHook(MidiHook *hook) {
  auto it = std::find(midiHooks.begin(), midiHooks.end(), hook);
  if (it != midiHooks.end()) {
    midiHooks.erase(it);
  }
}

/* Clears all the MidiHooks for this device.
 */
void MidiAgent::ClearMidiHooks() { midiHooks.clear(); }

/* Get this MidiAgent state as OBS Data. (includes midi hooks)
 * This is needed to Serialize the state in the config.
 * https://obsproject.com/docs/reference-settings.html
 */
obs_data_t *MidiAgent::GetData() {
  obs_data_t *data = obs_data_create();
  obs_data_set_string(data, "name", name.toStdString().c_str());
  obs_data_set_string(data, "outname", outname.toStdString().c_str());

  obs_data_set_bool(data, "enabled", enabled);
  obs_data_set_bool(data, "bidirectional", bidirectional);

  obs_data_array_t *arrayData = obs_data_array_create();
  for (int i = 0; i < midiHooks.size(); i++) {
    obs_data_t *hookData = midiHooks.at(i)->GetData();
    obs_data_array_push_back(arrayData, hookData);
  }
  obs_data_set_array(data, "hooks", arrayData);
  return data;
}
/*Handle OBS events*/
void MidiAgent::NewObsEvent(QString eventType, QString eventData) {
  if (!this->sending) {

    OBSDataAutoRelease data =
        obs_data_create_from_json(eventData.toStdString().c_str());

    MidiAgent *self = static_cast<MidiAgent *>(this);
    if (self->enabled == false || self->connected == false) {
      return;
    }

    // ON EVENT TYPE Find matching hook, pull data from that hook, and do thing.

    if (eventType == QString("SourceVolumeChanged")) {
      double vol = obs_data_get_double(data, "volume");
      uint8_t newvol = Utils::mapper2(cbrt(vol));
      QString source = obs_data_get_string(data, "sourceName");
      qDebug() << "OBS EVENT " << eventType << " " << source << " " << vol
               << " " << newvol;
      for (unsigned i = 0; i < self->midiHooks.size(); i++) {
        if (self->midiHooks.at(i)->command == "Set Volume" &&
            self->midiHooks.at(i)->param1 == source) {
          this->send(self->midiHooks.at(i)->type, self->midiHooks.at(i)->mchan,
                     self->midiHooks.at(i)->index, newvol);
        }
      }
    } else if (eventType == QString("SwitchScenes")) {
      QString source = obs_data_get_string(data, "scene-name");
      for (unsigned i = 0; i < self->midiHooks.size(); i++) {
        if (self->midiHooks.at(i)->command == "Set Current Scene" &&
            self->midiHooks.at(i)->param1 == source) {
          this->send("note_off", self->midiHooks.at(i)->mchan, lastscenebtn, 0);

          this->send("note_on", self->midiHooks.at(i)->mchan,
                     self->midiHooks.at(i)->index, 1);
          lastscenebtn = self->midiHooks.at(i)->index;
        }
      }

    } else if (eventType == QString("TransitionBegin")) {
      QString from = obs_data_get_string(data, "from-scene");
      for (unsigned i = 0; i < self->midiHooks.size(); i++) {
        if (self->midiHooks.at(i)->command == "Set Current Scene" &&
            self->midiHooks.at(i)->param1 == from) {
          this->send("note_on", self->midiHooks.at(i)->mchan,
                     self->midiHooks.at(i)->index, 0);
          this->send("note_off", self->midiHooks.at(i)->mchan,
                     self->midiHooks.at(i)->index, 0);
        }
      }

    } else if (eventType == QString("SourceMuteStateChanged")) {
      QString from = obs_data_get_string(data, "sourceName");
      for (unsigned i = 0; i < self->midiHooks.size(); i++) {
        if (self->midiHooks.at(i)->command == "Toggle Mute" &&
            self->midiHooks.at(i)->param1 == from) {
          bool muted = obs_data_get_bool(data, "muted");
          this->send("note_on", self->midiHooks.at(i)->mchan,
                     self->midiHooks.at(i)->index, muted);
        }
      }

    } else if (eventType == QString("SourceMuteStateChanged")) {
      QString from = obs_data_get_string(data, "sourceName");
      for (unsigned i = 0; i < self->midiHooks.size(); i++) {
        if (self->midiHooks.at(i)->command == "Set Mute" &&
            self->midiHooks.at(i)->param1 == from) {
          bool muted = obs_data_get_bool(data, "muted");
          this->send("note_on", self->midiHooks.at(i)->mchan,
                     self->midiHooks.at(i)->index, muted);
        }
      }
    } else if (eventType == QString("SourceRenamed")) {
      QString from = obs_data_get_string(data, "previousName");
      for (unsigned i = 0; i < self->midiHooks.size(); i++) {
        if (self->midiHooks.at(i)->param1 == from) {
          self->midiHooks.at(i)->param1 = obs_data_get_string(data, "newName");
          self->GetData();
        }
      }
    } else if (eventType == QString("Exiting")) {
      closing = true;

    } else if (eventType == QString("SourceDestroyed")) {
      if (!closing) {
        QString from = obs_data_get_string(data, "sourceName");

        for (unsigned i = 0; i < self->midiHooks.size(); i++) {
          if (self->midiHooks.at(i)->param1 == from) {
            self->RemoveMidiHook(self->midiHooks.at(i));
            self->GetData();
            i--;
          }
        }
        GetConfig()->Save();
      }
    }

    blog(1, "OBS EVENT- MidiAgent.cpp %s -- %s",
         eventType.toStdString().c_str(), eventData.toStdString().c_str());

  } else {
    this->sending = false;
  }
}
void MidiAgent::send(QString type, int channel, int norc, int value) {
  rtmidi::message *hello = new rtmidi::message();
  if (type == "control_change") {
    this->midiout->send_message(hello->control_change(channel, norc, value));
  } else if (type == "note_on") {
    this->midiout->send_message(hello->note_on(channel, norc, value));

  } else if (type == "note_off") {
    this->midiout->send_message(hello->note_off(channel, norc, value));
  }
}
