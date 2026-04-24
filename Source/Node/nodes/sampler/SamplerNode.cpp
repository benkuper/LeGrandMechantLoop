/*
  ==============================================================================

    SamplerNode.cpp
    Created: 14 Apr 2021 8:40:43am
    Author:  bkupe

  ==============================================================================
*/

#include "Node/NodeIncludes.h"

SamplerNode::SamplerNode(var params) :
    Node(getTypeStringStatic(), params, true, true, false, true, true, true),
    recordingNote(-1),
    lastRecordedNote(-1),
    lastPlayedNote(-1),
    recordedSamples(0),
    noteStatesCC("Notes"),
    viewStartKey(20),
    isUpdatingLibrary(false),
    controlsCC("Controls"),
    playCC("Play"),
    recordCC("Record"),
    adsrCC("ADSR"),
    libraryCC("Library"),
    curBankIndex(1)
{
    controlsCC.includeTriggersInSaveLoad = true;
    saveAndLoadRecursiveData = true;

    numChannels = playCC.addIntParameter("Channels", "Number of channels", 1, 1);
    monitor = playCC.addBoolParameter("Monitor", "Monitor input audio", true);
    playMode = playCC.addEnumParameter("Play Mode", "How samples are treated");
    playMode->addOption("Hit (Loop)", HIT_LOOP)->addOption("Hit (One shot)", HIT_ONESHOT)->addOption("Peek (Continuous)", PEEK)->addOption("Keep (Hold)", KEEP);

    hitMode = playCC.addEnumParameter("Hit Mode", "How playing a sample is handled.\nPiano is normal playing, releasing a key stops the sample.\nHit is starting the sample but releasing doesn't affect (only works in One Shot play mode) Full will not take in account playing again the note if the sample is still playing. Reset will allow to replay from start.\nToggle is alternating key presses to start/stop instead of using key release");
    hitMode->addOption("Piano", PIANO)->addOption("Hit (Full Sample)", HIT_FULL)->addOption("Hit (Reset)", HIT_RESET)->addOption("Toggle", TOGGLE);

    oneShotFadeTimeMS = playCC.addIntParameter("One Shot Fade MS", "Fade applied to the end of a hit/one shot sample (ms)", 10, 0, 1000);

    playNoteNumber = playCC.addIntParameter("Play Note", "Choose note to trigger", 60, 0, 127);
    playNoteTimeMS = playCC.addIntParameter("Play Note Time", "Duration (ms) before it sends a note off automatically. 0 means infinite until manually stopped.", 500, 0, 10000);
    playNoteTrigger = playCC.addTrigger("Trigger Note", "Trigger the chosen note");

    
    addChildControllableContainer(&playCC, false, 0);


    clearMode = controlsCC.addBoolParameter("Clear Mode", "When checked, recorded or played notes are cleared depending on clearing mode", false);
    clearLastMode = controlsCC.addEnumParameter("Clear Last Mode", "The way to clear when clear last is triggered");
    clearLastMode->addOption("Last Played", LAST_PLAYED)->addOption("Last Recorded", LAST_RECORDED);

    clearLastRecordedTrigger = controlsCC.addTrigger("Clear", "Clear Last recorded or played note");
    clearAllNotesTrigger = controlsCC.addTrigger("Clear All Notes", "Clear all recorded notes");

    revertLastMode = controlsCC.addEnumParameter("Revert Last Mode", "The way to revert when revert last is triggered");
    revertLastMode->addOption("Last Played", LAST_PLAYED)->addOption("Last Recorded", LAST_RECORDED);

    revertLastRecordedTrigger = controlsCC.addTrigger("Revert", "Revert Last recorded or played note");

    startAutoKey = controlsCC.addIntParameter("Start Auto Key", "The pitch to start auto key computation from", 0, 0, 127);
    endAutoKey = controlsCC.addIntParameter("End Auto Key", "The pitch to end auto key computation from", 127, 0, 127);
    
    autoKeyLiveMode = controlsCC.addBoolParameter("Auto Key", "If checked, hitting an unrecorded note will play it repitched from the closest found", false);
    autoKeyFadeTimeMS = controlsCC.addIntParameter("Auto Key Fade", "Fade for anticlick when repitching auto keys, in milliseconds", 50);

    autoKeyLoopFade = controlsCC.addBoolParameter("Auto Key Loop Fade", "If checked, generated auto-key samples are shortened and overlap-blended so the loop start and end match more cleanly.", false);
    autoKeyMode = controlsCC.addEnumParameter("Auto Key Algorithm", "Algorithm used to pitch-shift when computing auto keys.\nResample: clean, no wobble, slight formant shift.\nRubberBand: phase vocoder, preserves formants, may wobble on tonal sounds.");
    autoKeyMode->addOption("RubberBand", RUBBERBAND)->addOption("Resample", RESAMPLE);

    // --- New RubberBand Tweakable Parameters ---
    rbPreserveFormants = controlsCC.addBoolParameter("RB Formants", "Preserve Formants (RubberBand)", false);
    
    rbPitchMode = controlsCC.addEnumParameter("RB Pitch Mode", "Pitch shifting algorithm");
    rbPitchMode->addOption("High Quality", RB_PITCH_HQ)->addOption("High Consistency", RB_PITCH_CONSISTENT);
    
    rbTransients = controlsCC.addEnumParameter("RB Transients", "Transient handling");
    rbTransients->addOption("Crisp", RB_TRANS_CRISP)->addOption("Mixed", RB_TRANS_SMOOTH)->addOption("Smooth", RB_TRANS_SMOOTH);
    
    rbWindowSize = controlsCC.addEnumParameter("RB Window", "FFT Window Size");
    rbWindowSize->addOption("Standard", RB_WIN_STANDARD)->addOption("Short", RB_WIN_SHORT)->addOption("Long", RB_WIN_LONG);

    computeAutoKeysTrigger = controlsCC.addTrigger("Compute Auto Keys", "If checked, the auto keys will be computed from the recorded notes", true);
    
    
    addChildControllableContainer(&controlsCC, false, 1);

    fadeTimeMS = recordCC.addIntParameter("Fade Time", "Time to fade between start and end of recording, in milliseconds", 20);
    recVolumeThreshold = (FloatParameter*)recordCC.addParameter(new DecibelFloatParameter("Rec Volume Threshold", "Volume threshold to start recording", .3, false));
    recVolumeThreshold->canBeDisabledByUser = true;
    stopRecVolumeThreshold = (FloatParameter*)recordCC.addParameter(new DecibelFloatParameter("Stop Rec Volume Threshold", "Volume threshold to stop recording", .1, false));
    stopRecVolumeThreshold->canBeDisabledByUser = true;

    isRecording = recordCC.addBoolParameter("Is Recording", "Is recording a note ?", false);
    isRecording->setControllableFeedbackOnly(true);

    addChildControllableContainer(&recordCC, false, 2);

    attack = adsrCC.addFloatParameter("Attack", "Time of the attack in seconds", .01f, 0);
    attack->defaultUI = FloatParameter::TIME;

    attackCurve = adsrCC.addFloatParameter("Attack Curve", "Bend the attack", 0.06f, 0, 0.1f);

    decay = adsrCC.addFloatParameter("Decay", "Time of decay in seconds", .2f, 0);
    decay->defaultUI = FloatParameter::TIME;

    sustain = new DecibelFloatParameter("Sustain", "Sustain dB");
    adsrCC.addParameter(sustain);

    release = adsrCC.addFloatParameter("Release", "Time of the release in seconds", .2f, 0);
    release->defaultUI = FloatParameter::TIME;

    releaseCurve = adsrCC.addFloatParameter("Release Curve", "Bend the release", 0.001f, 0.001f, 0.1f);

    addChildControllableContainer(&adsrCC, false, 3);

    libraryFolder = libraryCC.addFileParameter("Lirbary Folder", "To export and import");
    libraryFolder->directoryMode = true;

    currentLibrary = libraryCC.addEnumParameter("Current Library", "Select a specific sub folder or the none for the root folder");
    currentBank = libraryCC.addIntParameter("Current Bank", "Index of the bank in the library folder, starting with 1. This will load a folder with a name starting with this number", 1, 1, 100);

    nextBank = libraryCC.addTrigger("Next Bank", "Goes to next bank");
    prevBank = libraryCC.addTrigger("Previous Bank", "Goes to previous bank");

    bankDescription = libraryCC.addStringParameter("Bank Description", "Description for this bank if available", "");

    autoLoadBank = libraryCC.addBoolParameter("Auto Load Bank", "If checked, the bank will be loaded when the current bank is changed", true);
    loadBankTrigger = libraryCC.addTrigger("Load Bank", "Load all samples from the current bank");
    saveBankTrigger = libraryCC.addTrigger("Save Bank", "Export all samples at once to this bank's folder");
    showFolderTrigger = libraryCC.addTrigger("Show Folder", "Show the folder in explorer");

    addChildControllableContainer(&libraryCC, false, 4);

    keyboardState.addListener(this);

    int sr = processor->getSampleRate();

    for (int i = 0; i < 128; i++)
    {
        SamplerNote* sn = new SamplerNote();
        sn->buffer.clear();
        sn->adsr.setAttackRate(attack->floatValue() * sr);
        sn->adsr.setTargetRatioA(attackCurve->floatValue());
        sn->adsr.setDecayRate(decay->floatValue() * sr);
        sn->adsr.setSustainLevel(sustain->gain);
        sn->adsr.setReleaseRate(release->floatValue() * sr);
        sn->adsr.setTargetRatioDR(releaseCurve->floatValue());

        sn->state = noteStatesCC.addEnumParameter(MidiMessage::getMidiNoteName(i, true, true, 3), "State for this note");
        sn->state->addOption("Empty", EMPTY)->addOption("OnSet", ONSET)->addOption("Recording", RECORDING)->addOption("Filled", FILLED)->addOption("Processing", PROCESSING)->addOption("Playing", PLAYING);
        sn->state->setControllableFeedbackOnly(true);

        sn->oneShotted = false;

        samplerNotes.add(sn);
    }

    updateBuffers();
    setAudioInputs(numChannels->intValue());
    setAudioOutputs(numChannels->intValue());

    addChildControllableContainer(&noteStatesCC);
    noteStatesCC.hideInEditor = true;

    setMIDIIO(true, true);
}

SamplerNode::~SamplerNode()
{
    samplerNotes.clear();
}

void SamplerNode::clearNote(int note)
{
    if (note == -1) return;
    ScopedSuspender sp(processor);
    samplerNotes[note]->adsr.reset();
    samplerNotes[note]->buffer.setSize(0, 0);
    samplerNotes[note]->state->setValueWithData(EMPTY);

}

void SamplerNode::revertNote(int note)
{
    if (note == -1 || note >= samplerNotes.size()) return;
    ScopedSuspender sp(processor);
    auto* sn = samplerNotes[note];
    if (sn->state->getValueDataAsEnum<NoteState>() != FILLED) return;

    for (int i = 0; i < sn->buffer.getNumChannels(); i++)
    {
        sn->buffer.reverse(i, 0, sn->buffer.getNumSamples());
    }
}

void SamplerNode::clearAllNotes()
{
    stopRecording();

    ScopedSuspender sp(processor);
    for (auto& n : samplerNotes)
    {
        n->adsr.reset();
        n->buffer.setSize(0, 0);
        n->state->setValueWithData(EMPTY);
    }
}

void SamplerNode::resetAllNotes()
{
    ScopedSuspender sp(processor);
    for (auto& n : samplerNotes)
    {
        n->reset();
    }
}

void SamplerNode::computeAutoKeys()
{
    for (int i = 0; i < samplerNotes.size(); i++)
    {
        NoteState ns = samplerNotes[i]->state->getValueDataAsEnum<NoteState>();
        if (ns == PROCESSING || samplerNotes[i]->isProxyNote()) clearNote(i);
    }

    int startKey = startAutoKey->intValue();
    int endKey = endAutoKey->intValue();
    
    // Grab UI settings to pass to threads
    bool formants = rbPreserveFormants->boolValue();
    int transients = rbTransients->getValueDataAsEnum<int>();
    int pitchMode = rbPitchMode->getValueDataAsEnum<int>();
    int windowSize = rbWindowSize->getValueDataAsEnum<int>();
    bool shouldLoopFade = autoKeyLoopFade->boolValue();

    for (int i = startKey; i <= endKey; i++)
    {
        SamplerNote* n = samplerNotes[i];
        if (n == nullptr) return;
        NoteState ns = n->state->getValueDataAsEnum<NoteState>();
        if (ns != EMPTY) continue;

        int closestNote = -1;
        {
            for (int j = 1; j <= 127; j++)
            {
                int lowI = i - j;
                int highI = i + j;
                if (lowI >= 0 && samplerNotes[lowI]->hasContent())
                {
                    closestNote = lowI;
                    break;
                }
                else if (highI < 128 && samplerNotes[highI]->hasContent())
                {
                    closestNote = highI;
                    break;
                }
            }
        }

        if (closestNote != -1)
        {
            double shift = MidiMessage::getMidiNoteInHertz(i) / MidiMessage::getMidiNoteInHertz(closestNote);
            n->computeAutoKey(samplerNotes[closestNote], shift, getFadeNumSamples(autoKeyFadeTimeMS->intValue()), autoKeyMode->getValueDataAsEnum<AutoKeyAlgorithm>(), formants, transients, pitchMode, windowSize, shouldLoopFade);
        }
    }
}


void SamplerNode::updateBuffers()
{
    ScopedSuspender sp(processor);
    updateRingBuffer();
    for (auto& n : samplerNotes)  n->buffer.setSize(getNumAudioInputs(), n->buffer.getNumSamples(), false, true);
}

void SamplerNode::updateRingBuffer()
{
    ScopedSuspender sp(processor);
    ringBuffer.reset(new RingBuffer<float>(getNumAudioInputs(), getFadeNumSamples(fadeTimeMS->intValue()) * 2));
}

void SamplerNode::startRecording(int note)
{
    if (recordingNote != -1) return;

    GenericScopedLock<SpinLock> lock(recLock);

    isRecording->setValue(true);
    recordingNote = note;

    int recNumSamples = processor->getSampleRate() * 60; // 1 min rec samples

    SamplerNote* samplerNote = samplerNotes[note];
    samplerNote->state->setValueWithData(RECORDING);

    samplerNote->buffer.setSize(getNumAudioInputs(), recNumSamples, false, true);

    int fadeNumSamples = getFadeNumSamples(fadeTimeMS->intValue());
    if (fadeNumSamples > 0)
    {
        preRecBuffer.setSize(samplerNote->buffer.getNumChannels(), fadeNumSamples, false, true);
        ringBuffer->readSamples(preRecBuffer, fadeNumSamples);
    }

    recordedSamples = 0;
}

void SamplerNode::stopRecording()
{
    if (recordingNote == -1) return;

    GenericScopedTryLock<SpinLock> lock(recLock);

    SamplerNote* samplerNote = samplerNotes[recordingNote];
    recordedSamples = recordedSamples - (recordedSamples % processor->getBlockSize()); //fit to blockSize

    if (recordedSamples > 0)
    {
        //fade with ring buffer using looper fadeTimeMS
        int fadeNumSamples = getFadeNumSamples(fadeTimeMS->intValue());
        samplerNote->buffer.setSize(samplerNote->buffer.getNumChannels(), recordedSamples, true);

        if (fadeNumSamples > 0)
        {
            int cropFadeNumSamples = jmin(recordedSamples, fadeNumSamples);
            int bufferStartSample = recordedSamples - cropFadeNumSamples;

            int preRecStartSample = preRecBuffer.getNumSamples() - cropFadeNumSamples;

            samplerNote->buffer.applyGainRamp(bufferStartSample, cropFadeNumSamples, 1, 0);

            for (int i = 0; i < samplerNote->buffer.getNumChannels(); i++)
            {
                samplerNote->buffer.addFromWithRamp(i, bufferStartSample, preRecBuffer.getReadPointer(i, preRecStartSample), cropFadeNumSamples, 0, 1);
            }

            preRecBuffer.clear();
        }

        samplerNote->state->setValueWithData(FILLED);
    }
    else
    {
        samplerNote->state->setValueWithData(EMPTY);
    }

    lastRecordedNote = recordingNote;
    lastPlayedNote = recordingNote;
    recordingNote = -1;
    isRecording->setValue(false);
}


void SamplerNode::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
    Node::onControllableFeedbackUpdateInternal(cc, c);

    if (c == numChannels)
    {
        setAudioInputs(numChannels->intValue());
        setAudioOutputs(numChannels->intValue());
        updateBuffers();
    }
    if (c == attack || c == decay || c == sustain || c == release || c == attackCurve || c == releaseCurve)
    {
        int sr = processor->getSampleRate();
        for (int i = 0; i < 128; i++)
        {
            if (c == attack) samplerNotes[i]->adsr.setAttackRate(attack->floatValue() * sr);
            else if (c == attackCurve) samplerNotes[i]->adsr.setTargetRatioA(attackCurve->floatValue());
            else if (c == decay) samplerNotes[i]->adsr.setDecayRate(decay->floatValue() * sr);
            else if (c == sustain) samplerNotes[i]->adsr.setSustainLevel(sustain->gain);
            else if (c == release) samplerNotes[i]->adsr.setReleaseRate(release->floatValue() * sr);
            else if (c == releaseCurve) samplerNotes[i]->adsr.setTargetRatioDR(releaseCurve->floatValue());
        }

    }
    else if (c == fadeTimeMS)
    {
        updateRingBuffer();
    }
    else if (c == libraryFolder)
    {
        updateLibraries();
    }
    else if (c == currentLibrary)
    {
        if (!isUpdatingLibrary)
        {
            String b = currentLibrary->getValueData().toString();
            if (b == "new")
            {
                AlertWindow* window = new AlertWindow("Add a Bank", "Add a new bank", AlertWindow::AlertIconType::NoIcon);
                window->addTextEditor("bank", libraryFolder->getFile().getNonexistentChildFile("Library", "", true).getFileName(), "Library Name");
                window->addButton("OK", 1, KeyPress(KeyPress::returnKey));
                window->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

                window->enterModalState(true, ModalCallbackFunction::create([window, this](int result)
                    {

                        if (result)
                        {
                            String bankName = window->getTextEditorContents("bank");
                            File f = libraryFolder->getFile().getChildFile(bankName);
                            if (f.exists())
                            {
                                LOGWARNING("Bank already exists with name " << f.getFileName());
                                return;
                            }

                            f.createDirectory();
                            updateLibraries(false);
                            currentLibrary->setValueWithKey(f.getFileName());
                        }
                    }
                ), true);
            }
            else
            {
                currentBank->setValue(1, false, true);
            }
        }
    }
    else if (c == currentBank)
    {
        updateBank();
    }
    else if (c == nextBank)
    {
        currentBank->setValue(currentBank->intValue() + 1);
    }
    else if (c == prevBank)
    {
        currentBank->setValue(currentBank->intValue() - 1);
    }
    else if (c == bankDescription)
    {
        if (!bankFolder.exists() && bankFolder != File()) bankFolder.createDirectory();
        File newFolder = bankFolder.getParentDirectory().getChildFile(currentBank->stringValue() + "-" + bankDescription->stringValue());
        bankFolder.moveFileTo(newFolder);
        bankFolder = newFolder;
    }
    else if (c == playMode)
    {
        resetAllNotes();
    }
    else if (c == clearLastRecordedTrigger)
    {
        if (clearLastMode->getValueDataAsEnum<ClearMode>() == LAST_RECORDED)
        {
            clearNote(lastRecordedNote);
            lastRecordedNote = -1;
        }
        else
        {
            clearNote(lastPlayedNote);
            lastPlayedNote = -1;
        }
    }
    else if (c == clearAllNotesTrigger)
    {
        clearAllNotes();
    }
    else if (c == revertLastRecordedTrigger)
    {
        if (revertLastMode->getValueDataAsEnum<ClearMode>() == LAST_RECORDED)
        {
            revertNote(lastRecordedNote);
        }
        else
        {
            revertNote(lastPlayedNote);
        }
    }
    else if (c == loadBankTrigger)
    {
        loadBankSamples();
    }
    else if (c == saveBankTrigger)
    {
        saveBankSamples();
    }
    else if (c == showFolderTrigger)
    {
        bankFolder.startAsProcess();
    }
    else if (c == computeAutoKeysTrigger)
    {
        computeAutoKeys();
    }
    else if (c == playNoteTrigger)
    {
        int note = playNoteNumber->intValue();
        handleNoteOn(&keyboardState, 1, note, 1.0f);
        
        int durationMS = playNoteTimeMS->intValue();
        if (durationMS > 0)
        {
            GenericScopedLock<SpinLock> lock(scheduledNoteOffsLock);
            int samples = durationMS * processor->getSampleRate() / 1000;
            // check if already scheduling this note off
            bool found = false;
            for (auto& sn : scheduledNoteOffs)
            {
                if (sn.first == note)
                {
                    sn.second = samples; // update duration
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                scheduledNoteOffs.add({note, samples});
            }
        }
    }
}

void SamplerNode::controllableStateChanged(Controllable* c)
{
    Node::controllableStateChanged(c);
}


void SamplerNode::handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    PlayMode pm = playMode->getValueDataAsEnum<PlayMode>();
    HitMode hm = hitMode->getValueDataAsEnum<HitMode>();

    SamplerNote* sn = samplerNotes[midiNoteNumber];
    NoteState ns = sn->state->getValueDataAsEnum<NoteState>();

    if (clearMode->boolValue())
    {
        clearNote(midiNoteNumber);
    }
    else if (ns == RECORDING)
    {
        //nothing
    }
    else if (!samplerNotes[midiNoteNumber]->hasContent())
    {
        int closestNote = -1;
        if (autoKeyLiveMode->boolValue())
        {
            for (int i = 1; i <= 127; i++)
            {
                int lowI = midiNoteNumber - i;
                int highI = midiNoteNumber + i;
                if (lowI >= 0 && samplerNotes[lowI]->hasContent())
                {
                    closestNote = lowI;
                    break;
                }
                else if (highI < 128 && samplerNotes[highI]->hasContent())
                {
                    closestNote = highI;
                    break;
                }
            }
        }

        if (closestNote != -1)
        {
            NoteState ns = sn->state->getValueDataAsEnum<NoteState>();
            if (hm == TOGGLE && ns == PLAYING)
            {
                samplerNotes[midiNoteNumber]->adsr.gate(0);
                samplerNotes[midiNoteNumber]->state->setValueWithData(FILLED);
            }
            else if (ns == PLAYING && (hm == HIT_RESET || hm == HIT_FULL))
            {
                if (hm == HIT_RESET)
                {
                    sn->adsr.reset();
                    sn->playingSample = 0;
                    sn->rtPitchReadSample = 0;
                    sn->applyStartRamp = true;
                }
            }
            else
            {
                double shift = MidiMessage::getMidiNoteInHertz(midiNoteNumber) / MidiMessage::getMidiNoteInHertz(closestNote);
                samplerNotes[midiNoteNumber]->setAutoKey(samplerNotes[closestNote], shift);

                sn->adsr.reset();
                sn->applyStartRamp = true;
                sn->velocity = velocity;
                sn->adsr.gate(1);
                lastPlayedNote = midiNoteNumber;
                sn->state->setValueWithData(PLAYING);
            }
        }
        else
        {
            if (recVolumeThreshold->enabled) sn->state->setValueWithData(ONSET);
            else startRecording(midiNoteNumber);
        }
    }
    else //filled note playing
    {
        NoteState ns = sn->state->getValueDataAsEnum<NoteState>();
        if (hm == TOGGLE && ns == PLAYING)
        {
            samplerNotes[midiNoteNumber]->adsr.gate(0);
            samplerNotes[midiNoteNumber]->state->setValueWithData(FILLED);
        }
        else if (ns == PLAYING && (hm == HIT_RESET || hm == HIT_FULL))
        {
            if (hm == HIT_RESET)
            {
                sn->adsr.reset();
                sn->playingSample = 0;
                sn->applyStartRamp = true;
            }
        }
        else
        {
            if (pm == HIT_LOOP || pm == HIT_ONESHOT)
            {
                // CLEAN RETRIGGER: Safely hand off the currently playing tail
                if (attack->floatValue() > 0.0f && !sn->isProxyNote() && sn->playingSample > 0 && !sn->oneShotted)
                {
                    sn->oldPlayingSample = sn->playingSample;
                    sn->oldFadeRemaining = (int)(AudioManager::getInstance()->currentSampleRate * 0.01f); // 10ms seamless crossfade
                    sn->oldFadeTotal = sn->oldFadeRemaining;
                    sn->oldFadeStartGain = sn->velocity * (float)sn->adsr.getOutput();
                }

                sn->playingSample = 0;
                sn->oneShotted = false;
                sn->applyStartRamp = true;
                sn->adsr.reset();
            }

            if (sn->isProxyNote())
            {
                sn->pitcher->reset();
                sn->rtPitchReadSample = sn->playingSample;
            }

            sn->velocity = velocity;
            sn->applyStartRamp = sn->applyStartRamp || sn->playingSample == 0;
            sn->adsr.gate(1);
            lastPlayedNote = midiNoteNumber;
            sn->state->setValueWithData(PLAYING);
        }
    }
}

void SamplerNode::handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    NoteState ns = samplerNotes[midiNoteNumber]->state->getValueDataAsEnum<NoteState>();
    if (ns == ONSET)
    {
        return;
    }

    HitMode hm = hitMode->getValueDataAsEnum<HitMode>();
    if (ns == RECORDING) stopRecording();
    else if (samplerNotes[midiNoteNumber]->hasContent() || samplerNotes[midiNoteNumber]->isProxyNote())
    {
        if (hm != PIANO) return;
        samplerNotes[midiNoteNumber]->adsr.gate(0);
        samplerNotes[midiNoteNumber]->state->setValueWithData(FILLED);
    }
}


int SamplerNode::getFadeNumSamples(int fadeMS)
{
    return fadeMS * AudioManager::getInstance()->currentSampleRate / 1000;
}

void SamplerNode::updateLibraries(bool loadAfter)
{
    File folder = libraryFolder->getFile();
    if (!folder.exists() || !folder.isDirectory()) return;

    var oldData = currentLibrary->getValueData();

    isUpdatingLibrary = true;
    currentLibrary->clearOptions();
    Array<File> files = folder.findChildFiles(File::findDirectories, false);
    for (auto& f : files)
    {
        currentLibrary->addOption(f.getFileName(), f.getFullPathName());
    }

    currentLibrary->addOption("Add new...", "new");

    if (oldData.isVoid()) currentLibrary->setValue(currentLibrary->enumValues[0]->key);
    else if (oldData.toString() != "new") currentLibrary->setValue(oldData);

    isUpdatingLibrary = false;

    if (loadAfter) currentBank->setValue(1, false, true);
}

void SamplerNode::updateBank()
{
    File libRootFolder = libraryFolder->getFile();
    if (!libRootFolder.exists() || !libRootFolder.isDirectory()) return;

    String libName = currentLibrary->getValueData().toString();
    if (libName.isEmpty())
    {
        NLOG(niceName, "No library selected in folder, please choose or create a library to load and save.");
        return;
    }

    File libFolder = libRootFolder.getChildFile(libName);
    File bFolder;
    Array<File> bFolders = libFolder.findChildFiles(File::findDirectories, false);

    String desc = "";
    for (auto& bf : bFolders)
    {
        StringArray split;
        split.addTokens(bf.getFileName(), "-", "");
        if (split[0] == String(currentBank->intValue()))
        {
            desc = split.size() > 0 ? split[1] : "";
            bFolder = bf;
            break;
        }
    }

    if (bFolder == File())
    {
        bFolder = libFolder.getChildFile(String(currentBank->intValue()));
        bFolder.createDirectory();
    }

    bankFolder = bFolder;
    bankDescription->setValue(desc);

    if (autoLoadBank->boolValue()) loadBankSamples();
}

void SamplerNode::saveBankSamples()
{
    if (!bankFolder.exists()) bankFolder.createDirectory();

    Array<File> filesToDelete = bankFolder.findChildFiles(File::TypesOfFileToFind::findFiles, false);
    for (auto& f : filesToDelete) f.deleteFile();
    bankFolder.createDirectory();

    LOG("Exporting samples...");

    if (!bankFolder.exists()) return;

    int numSamplesExported = 0;
    for (int i = 0; i < samplerNotes.size(); i++)
    {
        SamplerNote* n = samplerNotes[i];
        if (!n->hasContent() || n->isProxyNote()) continue;

        File f = bankFolder.getChildFile(String(i) + ".wav");
        if (f.exists()) f.deleteFile();

        if (auto fileStream = std::unique_ptr<FileOutputStream>(f.createOutputStream()))
        {
            WavAudioFormat wavFormat;
            if (std::unique_ptr<AudioFormatWriter> writer = std::unique_ptr<AudioFormatWriter>(wavFormat.createWriterFor(fileStream.get(), processor->getSampleRate(), n->buffer.getNumChannels(), 16, {}, 0)))
            {
                fileStream.release();
                bool result = writer->writeFromAudioSampleBuffer(n->buffer, 0, n->buffer.getNumSamples());
                if (!result) LOGWARNING("Error writing note " << i);
                else numSamplesExported++;
                writer->flush();
            }
            else
            {
                LOGWARNING("Could not create a writer for " << f.getFileName());
            }
        }
        else
        {
            LOGWARNING("Could not open " << f.getFileName() << " for writing");
        }
    }

    NLOG(niceName, numSamplesExported << " sampler notes exported to " << bankFolder.getFullPathName());
}

void SamplerNode::loadBankSamples()
{
    if (!bankFolder.exists() || !bankFolder.isDirectory())
    {
        NLOGWARNING(niceName, "Samples folder is not valid");
        return;
    }

    LOG("Importing samples...");

    int numSamplesImported = 0;
    for (int i = 0; i < samplerNotes.size(); i++)
    {
        SamplerNote* n = samplerNotes[i];
        if (n->hasContent()) clearNote(i);

        File f = bankFolder.getChildFile(String(i) + ".wav");

        if (!f.existsAsFile()) continue;

        if (auto fileStream = std::unique_ptr<FileInputStream>(f.createInputStream()))
        {
            WavAudioFormat wavFormat;
            if (std::unique_ptr<AudioFormatReader> reader = std::unique_ptr<AudioFormatReader>(wavFormat.createReaderFor(fileStream.get(), false)))
            {
                fileStream.release();
                n->buffer.setSize(reader->numChannels, reader->lengthInSamples);
                reader->read(n->buffer.getArrayOfWritePointers(), reader->numChannels, 0, n->buffer.getNumSamples());
                n->state->setValueWithData(FILLED);
                numSamplesImported++;
            }
        }
    }

    NLOG(niceName, numSamplesImported << " sampler notes imported from " << bankFolder.getFullPathName());
}


var SamplerNode::getJSONData(bool includeNonOverriden)
{
    var data = Node::getJSONData(includeNonOverriden);
    data.getDynamicObject()->setProperty("viewStartKey", viewStartKey);
    return data;
}

void SamplerNode::loadJSONDataItemInternal(var data)
{
    Node::loadJSONDataItemInternal(data);
    viewStartKey = data.getProperty("viewStartKey", viewStartKey);
}

void SamplerNode::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    if (sampleRate != 0) midiCollector.reset(sampleRate);

    for (int i = 0; i < 128; i++)
    {
        samplerNotes[i]->adsr.setAttackRate(attack->floatValue() * sampleRate);
        samplerNotes[i]->adsr.setTargetRatioA(attackCurve->floatValue());
        samplerNotes[i]->adsr.setDecayRate(decay->floatValue() * sampleRate);
        samplerNotes[i]->adsr.setSustainLevel(sustain->gain);
        samplerNotes[i]->adsr.setReleaseRate(release->floatValue() * sampleRate);
        samplerNotes[i]->adsr.setTargetRatioDR(releaseCurve->floatValue());
    }
}

void SamplerNode::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (fadeTimeMS->intValue() > 0) ringBuffer->writeSamples(buffer, 0, jmin(buffer.getNumSamples(), ringBuffer->bufferSize));

    int blockSize = buffer.getNumSamples();

    {
        GenericScopedTryLock<SpinLock> lock(scheduledNoteOffsLock);
        if (lock.isLocked() && scheduledNoteOffs.size() > 0)
        {
            for (int i = scheduledNoteOffs.size() - 1; i >= 0; i--)
            {
                scheduledNoteOffs.getReference(i).second -= blockSize;
                if (scheduledNoteOffs.getReference(i).second <= 0)
                {
                    int note = scheduledNoteOffs.getReference(i).first;
                    handleNoteOff(&keyboardState, 1, note, 1.0f);
                    scheduledNoteOffs.remove(i);
                }
            }
        }
    }

    if (isRecording->boolValue())
    {
        GenericScopedTryLock<SpinLock> lock(recLock);

        if (lock.isLocked())
        {
            SamplerNote* samplerNote = samplerNotes[recordingNote];

            if (samplerNote->buffer.getNumSamples() >= blockSize)
            {
                for (int i = 0; i < buffer.getNumChannels(); i++)
                {
                    samplerNote->buffer.copyFrom(i, recordedSamples, buffer, i, 0, blockSize);
                }
            }

            recordedSamples += blockSize;
        }

        if (stopRecVolumeThreshold->enabled)
        {
            float mag = buffer.getMagnitude(0, blockSize);
            float decibels = Decibels::gainToDecibels(mag, -100.f);
            float normDecibels = jmap(decibels, -100.f, 6.f, 0.f, 1.f);
            if (normDecibels < stopRecVolumeThreshold->floatValue())
            {
                stopRecording();
            }
        }
    }

    AudioSampleBuffer onSetCheckBuffer(buffer.getNumChannels(), buffer.getNumSamples());
    onSetCheckBuffer.makeCopyOf(buffer);

    if (!monitor->boolValue()) buffer.clear();
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), false);

    AudioSampleBuffer tmpNoteBuffer(buffer.getNumChannels(), blockSize);

    PlayMode pm = playMode->getValueDataAsEnum<PlayMode>();

    for (int i = 0; i < 128; i++)
    {
        SamplerNote* s = samplerNotes[i];
        if (s == nullptr) continue;
        NoteState st = s->state->getValueDataAsEnum<NoteState>();
        if (st == EMPTY || st == RECORDING || st == PROCESSING) continue;

        if (st == ONSET)
        {
            if (isRecording->boolValue()) continue;
            float mag = onSetCheckBuffer.getMagnitude(0, blockSize);

            float decibels = Decibels::gainToDecibels(mag, -100.f);
            float normDecibels = jmap(decibels, -100.f, 6.f, 0.f, 1.f);
            if (normDecibels > recVolumeThreshold->floatValue())
            {
                startRecording(i);
            }
            continue;
        }

        if (s->adsr.getState() == CurvedADSR::env_idle)
        {
            if (pm == PEEK)
            {
                s->playingSample += blockSize;
                if (s->playingSample >= s->buffer.getNumSamples()) s->playingSample = 0;
            }
            else if (s->isProxyNote())
            {
                s->setAutoKey(nullptr);
                s->state->setValueWithData(EMPTY);
            }
            continue;
        }


        // ====================================================================================
        // CLEAN PLAYBACK ENGINE
        // ====================================================================================

        // 1. Process Retrigger Tail (Smoothly fade out the old sound directly to output)
        if (!s->isProxyNote() && s->oldFadeRemaining > 0 && s->oldPlayingSample >= 0)
        {
            int availableInSample = s->buffer.getNumSamples() - s->oldPlayingSample;
            int processLen = jmin(blockSize, jmin(s->oldFadeRemaining, availableInSample));

            if (processLen > 0)
            {
                float startGain = s->oldFadeStartGain * ((float)s->oldFadeRemaining / s->oldFadeTotal);
                float endGain   = s->oldFadeStartGain * ((float)(s->oldFadeRemaining - processLen) / s->oldFadeTotal);

                for (int j = 0; j < buffer.getNumChannels(); j++)
                {
                    int ch = j % s->buffer.getNumChannels();
                    buffer.addFromWithRamp(j, 0, s->buffer.getReadPointer(ch, s->oldPlayingSample), processLen, startGain, endGain);
                }
                s->oldPlayingSample += processLen;
                s->oldFadeRemaining -= processLen;
                if (s->oldFadeRemaining <= 0) s->oldFadeStartGain = 0.0f;
            }
            else
            {
                s->oldFadeRemaining = 0; // Tail has safely finished fading
                s->oldFadeStartGain = 0.0f;
            }
        }

        // 2. Process Main Playhead
        if (pm == HIT_ONESHOT && s->oneShotted)
        {
            // The sample is finished playing, but the ADSR release phase is still running to clear silence.
            tmpNoteBuffer.clear();
            s->adsr.applyEnvelopeToBuffer(tmpNoteBuffer, 0, blockSize);
            for (int j = 0; j < buffer.getNumChannels(); j++)
                buffer.addFrom(j, 0, tmpNoteBuffer.getReadPointer(j), blockSize, s->velocity);
            continue;
        }

        AudioSampleBuffer* targetBuffer = &s->buffer;
        int targetReadSample = s->playingSample;

        // Process real-time RubberBand Proxy if needed
        if (s->isProxyNote())
        {
            AudioBuffer<float> tmpBuffer(s->autoKeyFromNote->buffer.getNumChannels(), blockSize);
            auto readPointers = tmpBuffer.getArrayOfReadPointers();

            GenericScopedLock pLock(s->pitcherLock);
            while (s->pitcher->available() < blockSize)
            {
                const int sourceNumSamples = s->autoKeyFromNote->buffer.getNumSamples();
                for (int ch = 0; ch < buffer.getNumChannels(); ch++)
                {
                    const int firstPart = jmin(blockSize, sourceNumSamples - s->rtPitchReadSample);
                    tmpBuffer.copyFrom(ch, 0, s->autoKeyFromNote->buffer, ch, s->rtPitchReadSample, firstPart);
                    if (firstPart < blockSize)
                        tmpBuffer.copyFrom(ch, firstPart, s->autoKeyFromNote->buffer, ch, 0, blockSize - firstPart);
                }

                s->pitcher->process(readPointers, tmpBuffer.getNumSamples(), false);
                s->rtPitchReadSample = (s->rtPitchReadSample + blockSize) % sourceNumSamples;

                if (pm == HIT_ONESHOT && s->playingSample + blockSize >= sourceNumSamples)
                {
                    s->oneShotted = true;
                    s->playingSample = 0;
                    s->state->setValueWithData(s->hasContent() ? NoteState::FILLED : NoteState::EMPTY);
                    break;
                }
            }
            s->pitcher->retrieve(s->rtPitchedBuffer.getArrayOfWritePointers(), s->rtPitchedBuffer.getNumSamples());
            targetBuffer = &s->rtPitchedBuffer;
            targetReadSample = 0;
        }

        const int bufNumSamples = targetBuffer->getNumSamples();
        const int firstPart = jmax(0, jmin(blockSize, bufNumSamples - targetReadSample));
        const int secondPart = blockSize - firstPart;

        for (int j = 0; j < buffer.getNumChannels(); j++)
        {
            tmpNoteBuffer.clear(j, 0, blockSize);
            int ch = j % targetBuffer->getNumChannels();

            if (pm == HIT_ONESHOT || s->isProxyNote())
            {
                if (firstPart > 0)
                {
                    tmpNoteBuffer.copyFrom(j, 0, *targetBuffer, ch, targetReadSample, firstPart);

                    // End-of-Sample Anti-Click Ramp (Pure Buffer Math)
                    int fadeSamples = (int)(AudioManager::getInstance()->currentSampleRate * oneShotFadeTimeMS->intValue() / 1000.0f);
                    if (fadeSamples <= 0) fadeSamples = 64; // Enforce a 1.5ms minimum physical fade to prevent clicking
                    
                    int fadeStartSample = bufNumSamples - fadeSamples;
                    
                    // Are we currently reading inside the end-fade zone?
                    if (targetReadSample + firstPart > fadeStartSample)
                    {
                        int startLocal = jmax(0, fadeStartSample - targetReadSample);
                        int length = firstPart - startLocal;
                        
                        float startGain = 1.0f - ((float)jmax(0, targetReadSample + startLocal - fadeStartSample) / fadeSamples);
                        float endGain   = 1.0f - ((float)(targetReadSample + firstPart - fadeStartSample) / fadeSamples);
                        
                        tmpNoteBuffer.applyGainRamp(j, startLocal, length, jmax(0.0f, startGain), jmax(0.0f, endGain));
                    }
                }
            }
            else // LOOP MODE
            {
                if (firstPart > 0) tmpNoteBuffer.copyFrom(j, 0, *targetBuffer, ch, targetReadSample, firstPart);
                if (secondPart > 0) tmpNoteBuffer.copyFrom(j, firstPart, *targetBuffer, ch, 0, secondPart);
            }

            // Start-of-Sample Anti-Click Ramp (Pure Buffer Math)
            if (s->applyStartRamp && attack->floatValue() > 0.0f && targetReadSample == 0 && firstPart > 0)
            {
                int rampLen = jmin(firstPart, (int)(AudioManager::getInstance()->currentSampleRate * 0.001f)); // 1ms
                if (rampLen > 0) tmpNoteBuffer.applyGainRamp(j, 0, rampLen, 0.0f, 1.0f);
            }
        }

        s->applyStartRamp = false;

        // Apply ADSR and Add to Master Output
        s->adsr.applyEnvelopeToBuffer(tmpNoteBuffer, 0, blockSize);
        for (int j = 0; j < buffer.getNumChannels(); j++)
        {
            buffer.addFrom(j, 0, tmpNoteBuffer.getReadPointer(j), blockSize, s->velocity);
        }

        // Advance Playhead
        s->playingSample += blockSize;

        if (s->playingSample >= bufNumSamples)
        {
            if (pm == HIT_ONESHOT || s->isProxyNote())
            {
                s->oneShotted = true;
                s->adsr.gate(0);
                s->playingSample = 0;
            }
            else
            {
                s->playingSample %= bufNumSamples; // Loop back
            }
        }
    }
}


BaseNodeViewUI* SamplerNode::createViewUI()
{
    return new SamplerNodeViewUI(this);
}

SamplerNode::SamplerNote::SamplerNote() :
    Thread("Sampler Stretcher")
{
}

SamplerNode::SamplerNote::~SamplerNote()
{
    stopThread(1000);
}

void SamplerNode::SamplerNote::setAutoKey(SamplerNote* remoteNote, double shift)
{
    autoKeyFromNote = remoteNote;
    if (autoKeyFromNote == nullptr)
    {
        rtPitchedBuffer.clear();
        return;
    }

    GenericScopedLock lock(pitcherLock);

    if (pitcher == nullptr)
    {
        pitcher.reset(new RubberBand::RubberBandStretcher(Transport::getInstance()->sampleRate, autoKeyFromNote->buffer.getNumChannels(),
            RubberBand::RubberBandStretcher::OptionProcessRealTime
            | RubberBand::RubberBandStretcher::OptionStretchPrecise
            | RubberBand::RubberBandStretcher::OptionWindowShort
            | RubberBand::RubberBandStretcher::OptionTransientsMixed
            | RubberBand::RubberBandStretcher::OptionPitchHighConsistency
            | RubberBand::RubberBandStretcher::OptionChannelsTogether
        ));
    }

    pitcher->reset();
    pitcher->setMaxProcessSize(Transport::getInstance()->blockSize);
    pitcher->setPitchScale(shift);
    rtPitchReadSample = 0;
    rtPitchedBuffer.setSize(autoKeyFromNote->buffer.getNumChannels(), Transport::getInstance()->blockSize);
}

void SamplerNode::SamplerNote::computeAutoKey(SamplerNote* remoteNote, double shift, int fadeSamples, int algorithm, bool formants, int transients, int pitchMode, int windowSize, bool shouldLoopFade)
{
    shifting = shift;
    fadeNumSamples = fadeSamples;
    autoKeyAlgorithm = algorithm;
    autoKeyFromNote = remoteNote;
    
    optFormants = formants;
    optTransients = transients;
    optPitchMode = pitchMode;
    optWindow = windowSize;
    loopFadeEnabled = shouldLoopFade;
    
    startThread();
}

void SamplerNode::SamplerNote::run()
{
    state->setValueWithData(NoteState::PROCESSING);

    SamplerNote* sourceNote = autoKeyFromNote;
    if (sourceNote == nullptr || !sourceNote->hasContent())
    {
        buffer.setSize(0, 0);
        autoKeyFromNote = nullptr;
        state->setValueWithData(NoteState::EMPTY);
        return;
    }

    const int numChannels = sourceNote->buffer.getNumChannels();
    const int numSamples = sourceNote->buffer.getNumSamples();
    if (numChannels <= 0 || numSamples <= 0)
    {
        buffer.setSize(0, 0);
        autoKeyFromNote = nullptr;
        state->setValueWithData(NoteState::EMPTY);
        return;
    }

    const double safeShift = (std::isfinite(shifting) && shifting > 0.0) ? shifting : 1.0;

    AudioBuffer<float> sourceCopy(numChannels, numSamples);
    sourceCopy.makeCopyOf(sourceNote->buffer, true);

    AudioBuffer<float> pitchedFull(numChannels, numSamples);
    pitchedFull.clear();
    AudioBuffer<float> loopContinuation(numChannels, 0);
    int renderedNumSamples = numSamples;

    if (autoKeyAlgorithm == SamplerNode::RESAMPLE)
    {
        const int outSamples = (int)(numSamples / safeShift);
        
        const int loopFadeSamples = loopFadeEnabled
            ? jmin(jmax(0, numSamples - 1), fadeNumSamples)
            : 0;
            
        const int paddedInputSamples = numSamples + loopFadeSamples;
        const int outLoopFadeSamples = (int)(loopFadeSamples / safeShift);
        const int totalOutSamples = outSamples + outLoopFadeSamples;

        // Provide enough source samples plus interpolation margin
        const int srcNeeded = paddedInputSamples + 16;
        AudioBuffer<float> extSrc(numChannels, srcNeeded);
        extSrc.clear();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < numSamples; ++i)
                extSrc.setSample(ch, i, sourceCopy.getSample(ch, i));
            if (loopFadeEnabled && loopFadeSamples > 0)
                for (int i = 0; i < loopFadeSamples; ++i)
                    extSrc.setSample(ch, numSamples + i, sourceCopy.getSample(ch, i));
        }

        pitchedFull.setSize(numChannels, outSamples);
        pitchedFull.clear();
        
        AudioBuffer<float> totalOutput(numChannels, totalOutSamples);
        totalOutput.clear();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            CatmullRomInterpolator resampler;
            resampler.process(safeShift, extSrc.getReadPointer(ch), totalOutput.getWritePointer(ch), totalOutSamples);
        }
        
        for (int ch = 0; ch < numChannels; ++ch)
        {
            pitchedFull.copyFrom(ch, 0, totalOutput, ch, 0, outSamples);
        }
        
        renderedNumSamples = outSamples;

        if (loopFadeEnabled && outLoopFadeSamples > 0)
        {
            loopContinuation.setSize(numChannels, outLoopFadeSamples);
            for (int ch = 0; ch < numChannels; ++ch)
            {
                loopContinuation.copyFrom(ch, 0, totalOutput, ch, outSamples, outLoopFadeSamples);
            }
        }
    }
    else
    {
        // 1. Build Options dynamically from UI selections
        int rbOptions = RubberBand::RubberBandStretcher::OptionProcessOffline
                      | RubberBand::RubberBandStretcher::OptionStretchPrecise
                      | RubberBand::RubberBandStretcher::OptionChannelsTogether;

        if (optFormants) rbOptions |= RubberBand::RubberBandStretcher::OptionFormantPreserved;
        
        if (optTransients == SamplerNode::RB_TRANS_CRISP) rbOptions |= RubberBand::RubberBandStretcher::OptionTransientsCrisp;
        else if (optTransients == SamplerNode::RB_TRANS_MIXED) rbOptions |= RubberBand::RubberBandStretcher::OptionTransientsMixed;
        else if (optTransients == SamplerNode::RB_TRANS_SMOOTH) rbOptions |= RubberBand::RubberBandStretcher::OptionTransientsSmooth;
        
        if (optPitchMode == SamplerNode::RB_PITCH_HQ) rbOptions |= RubberBand::RubberBandStretcher::OptionPitchHighQuality;
        else if (optPitchMode == SamplerNode::RB_PITCH_CONSISTENT) rbOptions |= RubberBand::RubberBandStretcher::OptionPitchHighConsistency;
        
        if (optWindow == SamplerNode::RB_WIN_SHORT) rbOptions |= RubberBand::RubberBandStretcher::OptionWindowShort;
        else if (optWindow == SamplerNode::RB_WIN_LONG) rbOptions |= RubberBand::RubberBandStretcher::OptionWindowLong;


        RubberBand::RubberBandStretcher rubberBand(
            Transport::getInstance()->sampleRate,
            (size_t)numChannels,
            rbOptions
        );

        rubberBand.setPitchScale(safeShift);
        rubberBand.setTimeRatio(1.0);

        // In loop-fade mode we append a short post-roll from the start so the
        // rendered note contains the material needed to close the loop seam.
        const int loopFadeSamples = loopFadeEnabled
            ? jmin(jmax(0, numSamples - 1), fadeNumSamples)
            : 0;
        const int paddedInputSamples = numSamples + loopFadeSamples;

        AudioBuffer<float> paddedSource(numChannels, paddedInputSamples);
        paddedSource.clear();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            paddedSource.copyFrom(ch, 0, sourceCopy, ch, 0, numSamples);

            if (loopFadeEnabled && loopFadeSamples > 0)
            {
                paddedSource.copyFrom(ch, numSamples, sourceCopy, ch, 0, loopFadeSamples);
            }
        }

        rubberBand.setExpectedInputDuration((size_t)paddedInputSamples);

        // 3. Process the audio
        auto readPointers = paddedSource.getArrayOfReadPointers();
        rubberBand.study(readPointers, (size_t)paddedInputSamples, true);
        rubberBand.process(readPointers, (size_t)paddedInputSamples, true);

        // 4. Retrieve exactly paddedInputSamples and ignore the rest
        int collected = 0;
        const int chunkSize = 8192;
        AudioBuffer<float> fetchBuffer(numChannels, chunkSize);
        AudioBuffer<float> fullOutput(numChannels, paddedInputSamples);

        while (collected < paddedInputSamples && rubberBand.available() > 0)
        {
            int avail = (int)rubberBand.available();
            int toFetch = jmin(avail, chunkSize);
            int retrieved = (int)rubberBand.retrieve(fetchBuffer.getArrayOfWritePointers(), (size_t)toFetch);
            
            if (retrieved > 0)
            {
                int actualToKeep = jmin(retrieved, paddedInputSamples - collected);
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    fullOutput.copyFrom(ch, collected, fetchBuffer, ch, 0, actualToKeep);
                }
                collected += actualToKeep;
            }
            else break;
        }

        // 5. Fill any remaining tiny gap with absolute silence
        if (collected < paddedInputSamples)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                fullOutput.clear(ch, collected, paddedInputSamples - collected);
            }
        }

        // 6. Copy the requested note body. Loop overlap is applied afterward so
        // both algorithms share the same seam treatment.
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const int availableMainSamples = jmin(numSamples, collected);
            if (availableMainSamples > 0)
                pitchedFull.copyFrom(ch, 0, fullOutput, ch, 0, availableMainSamples);
        }

        renderedNumSamples = jmax(0, jmin(numSamples, collected));

        if (loopFadeEnabled && loopFadeSamples > 0)
        {
            const int availableContinuation = jmin(loopFadeSamples, jmax(0, collected - renderedNumSamples));
            if (availableContinuation > 0)
            {
                loopContinuation.setSize(numChannels, availableContinuation);
                for (int ch = 0; ch < numChannels; ++ch)
                    loopContinuation.copyFrom(ch, 0, fullOutput, ch, renderedNumSamples, availableContinuation);
            }
        }
    }

    if (loopFadeEnabled)
    {
        const int crossfadeSamples = jmin(jmax(0, renderedNumSamples / 2), fadeNumSamples);
        if (crossfadeSamples > 0)
        {
            const int loopedNumSamples = renderedNumSamples - crossfadeSamples;
            AudioBuffer<float> loopedBuffer(numChannels, loopedNumSamples);
            loopedBuffer.clear();
            const int bodySamples = jmax(0, loopedNumSamples - crossfadeSamples);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                if (bodySamples > 0)
                    loopedBuffer.copyFrom(ch, 0, pitchedFull, ch, crossfadeSamples, bodySamples);

                for (int i = 0; i < crossfadeSamples; ++i)
                {
                    const float alpha = crossfadeSamples == 1 ? 1.0f : (float)i / (float)(crossfadeSamples - 1);
                    const float tailGain = std::cos(alpha * MathConstants<float>::halfPi);
                    const float headGain = std::sin(alpha * MathConstants<float>::halfPi);
                    const float tailSample = loopContinuation.getNumSamples() > i
                        ? loopContinuation.getSample(ch, i)
                        : pitchedFull.getSample(ch, i);
                    const float headSample = pitchedFull.getSample(ch, loopedNumSamples + i);
                    loopedBuffer.setSample(ch, bodySamples + i, headSample * tailGain + tailSample * headGain);
                }
            }

            pitchedFull.makeCopyOf(loopedBuffer);
            renderedNumSamples = loopedNumSamples;
        }
    }
    else if (renderedNumSamples > 0 && renderedNumSamples < pitchedFull.getNumSamples())
    {
        AudioBuffer<float> trimmedBuffer(numChannels, renderedNumSamples);
        trimmedBuffer.makeCopyOf(pitchedFull, true);
        trimmedBuffer.setSize(numChannels, renderedNumSamples, true, false, true);
        pitchedFull.makeCopyOf(trimmedBuffer);
    }

    // Replace the buffer
    buffer.makeCopyOf(pitchedFull);
    
    autoKeyFromNote = nullptr;
    state->setValueWithData(NoteState::FILLED);
}


void SamplerNode::SamplerNote::reset()
{
    adsr.gate(0);
    setAutoKey(nullptr);
    oldPlayingSample = -1;
    oldFadeTotal = 0;
    oldFadeRemaining = 0;
    oldFadeStartGain = 0.0f;
    playingSample = 0;
    oneShotted = false;
    applyStartRamp = false;
    state->setValueWithData(hasContent() ? NoteState::FILLED : NoteState::EMPTY);
}
