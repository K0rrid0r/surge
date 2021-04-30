/*
** Surge Synthesizer is Free and Open Source Software
**
** Surge is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Surge was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Surge
** open source in September 2018.
*/

#include "ModulationEditor.h"
#include "SurgeSynthesizer.h"
#include "SurgeGUIEditor.h"
#include "RuntimeFont.h"
#include <sstream>

struct ModulationListBoxModel : public juce::ListBoxModel
{
    struct Datum
    {
        enum RowType
        {
            HEADER,
            SHOW_ROW,
            EDIT_ROW
        } type;

        explicit Datum(RowType t) : type(t) {}

        std::string hLab;
        int dest_id = -1, source_id = -1;
        std::string dest_name, source_name;
        int modNum;
    };
    std::vector<Datum> rows;
    std::string debugRows;

    struct HeaderComponent : public juce::Component
    {
        HeaderComponent(ModulationListBoxModel *mod, int row) : mod(mod), row(row) {}
        void paint(juce::Graphics &g) override
        {
            g.fillAll(juce::Colour(30, 30, 30));
            auto r = getBounds().expanded(-1);
            g.setFont(Surge::GUI::getFontManager()->getLatoAtSize(10, VSTGUI::kBoldFace));
            g.setColour(juce::Colour(255, 255, 255));
            g.drawText(mod->rows[row].hLab, r, juce::Justification::left);
        }
        ModulationListBoxModel *mod;
        int row;
    };

    struct ShowComponent : public juce::Component
    {
        ShowComponent(ModulationListBoxModel *mod, int row) : mod(mod), row(row) {}
        void paint(juce::Graphics &g) override
        {
            auto r = getBounds().withTrimmedLeft(15);

            g.setColour(juce::Colour(50, 50, 50));
            if (mod->rows[row].modNum % 2)
                g.setColour(juce::Colour(75, 75, 90));
            g.fillRect(r);

            auto er = r.expanded(-1);
            g.setFont(Surge::GUI::getFontManager()->getLatoAtSize(9));
            g.setColour(juce::Colour(255, 255, 255));
            g.drawText(mod->rows[row].source_name, r, juce::Justification::left);
            auto rR = r.withTrimmedRight(r.getWidth() / 2);
            g.drawText(mod->rows[row].dest_name, rR, juce::Justification::right);
        }
        ModulationListBoxModel *mod;
        int row;
    };

    struct EditComponent : public juce::Component
    {
        EditComponent(ModulationListBoxModel *mod, int row) : mod(mod), row(row) {}
        void paint(juce::Graphics &g) override
        {
            auto r = getBounds().withTrimmedLeft(15);

            g.setColour(juce::Colour(50, 50, 50));
            if (mod->rows[row].modNum % 2)
                g.setColour(juce::Colour(75, 75, 90));

            g.fillRect(r);

            auto er = r.expanded(-1);
            g.setFont(Surge::GUI::getFontManager()->getLatoAtSize(9));
            g.setColour(juce::Colour(200, 200, 255));
            g.drawText("Edit Controls Coming Soon", r, juce::Justification::left);
        }
        ModulationListBoxModel *mod;
        int row;
    };

    ModulationListBoxModel(ModulationEditor *ed) : moded(ed) { updateRows(); }

    void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                          bool rowIsSelected) override
    {
        g.fillAll(juce::Colour(0, 0, 0));
    }

    int getNumRows() override { return rows.size(); }

    void updateRows()
    {
        std::ostringstream oss;
        int modNum = 0;
        auto append = [&oss, &modNum, this](const std::string &type,
                                            const std::vector<ModulationRouting> &r) {
            if (r.empty())
                return;

            oss << type << "\n";
            auto h = Datum(Datum::HEADER);
            h.hLab = type;
            rows.push_back(h);

            char nm[TXT_SIZE], dst[TXT_SIZE];
            for (auto q : r)
            {
                SurgeSynthesizer::ID ptagid;
                if (moded->synth->fromSynthSideId(q.destination_id, ptagid))
                    moded->synth->getParameterName(ptagid, nm);
                std::string sname = moded->ed->modulatorName(q.source_id, false);
                auto rDisp = Datum(Datum::SHOW_ROW);
                rDisp.source_id = q.source_id;
                rDisp.dest_id = q.destination_id;
                rDisp.source_name = sname;
                rDisp.dest_name = nm;
                rDisp.modNum = modNum++;
                rows.push_back(rDisp);

                rDisp.type = Datum::EDIT_ROW;
                rows.push_back(rDisp);

                oss << "    > " << this->moded->ed->modulatorName(q.source_id, false) << " to "
                    << nm << " at " << q.depth << "\n";
            }
        };
        append("Global", moded->synth->storage.getPatch().modulation_global);
        append("Scene A Voice", moded->synth->storage.getPatch().scene[0].modulation_voice);
        append("Scene A Scene", moded->synth->storage.getPatch().scene[0].modulation_scene);
        append("Scene B Voice", moded->synth->storage.getPatch().scene[1].modulation_voice);
        append("Scene B Scene", moded->synth->storage.getPatch().scene[1].modulation_scene);
        debugRows = oss.str();
    }

    juce::Component *refreshComponentForRow(int rowNumber, bool isRowSelected,
                                            juce::Component *existingComponentToUpdate) override
    {
        auto row = existingComponentToUpdate;
        if (rowNumber >= 0 && rowNumber < rows.size())
        {
            if (row)
            {
                bool replace = false;
                switch (rows[rowNumber].type)
                {
                case Datum::HEADER:
                    replace = !dynamic_cast<HeaderComponent *>(row);
                    break;
                case Datum::SHOW_ROW:
                    replace = !dynamic_cast<ShowComponent *>(row);
                    break;
                case Datum::EDIT_ROW:
                    replace = !dynamic_cast<EditComponent *>(row);
                    break;
                }
                if (replace)
                {
                    delete row;
                    row = nullptr;
                }
            }
            if (!row)
            {
                switch (rows[rowNumber].type)
                {
                case Datum::HEADER:
                    row = new HeaderComponent(this, rowNumber);
                    break;
                case Datum::SHOW_ROW:
                    row = new ShowComponent(this, rowNumber);
                    break;
                case Datum::EDIT_ROW:
                    row = new EditComponent(this, rowNumber);
                    break;
                }
            }
        }
        else
        {
            if (row)
            {
                // Nothing to display, free the custom component
                delete row;
                row = nullptr;
            }
        }

        return row;
    }

    ModulationEditor *moded;
};

ModulationEditor::ModulationEditor(SurgeGUIEditor *ed, SurgeSynthesizer *s)
    : ed(ed), synth(s), juce::Component("Modulation Editor")
{
    setSize(750, 450); // FIXME

    listBoxModel = std::make_unique<ModulationListBoxModel>(this);
    listBox = std::make_unique<juce::ListBox>("Mod Editor List", listBoxModel.get());
    listBox->setBounds(5, 5, 740, 440);
    listBox->setRowHeight(18);
    addAndMakeVisible(*listBox);

    /*textBox = std::make_unique<juce::TextEditor>("Mod editor text box");
    textBox->setMultiLine(true, false);
    textBox->setBounds(5, 355, 740, 90);
    textBox->setFont(Surge::GUI::getFontManager()->getLatoAtSize(9));
    addAndMakeVisible(*textBox);
    textBox->setText(listBoxModel->debugRows);*/
}

ModulationEditor::~ModulationEditor() = default;