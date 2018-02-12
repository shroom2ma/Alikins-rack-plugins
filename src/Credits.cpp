#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

#include "alikins.hpp"
#include "dsp/digital.hpp"
#include "util.hpp"

/*
 * Store and display author info and metadata
 */

/* TODO:
    show the credits info
     (optional? could make it a tiny 1hp data only)
    methods to detect changes
    methods to detect new authors
 */

/*
 * credits model
 *
 * 'credits' key in json data
 *
 * CreditList - list of Credit()
 *
 * Credit
 *  .authorList
 *      Author()
 *          string/unicode name
 *          string/unicode url
 *          # author_id is only uniq and meaningful within a particular doc
 *          # ie, not globally uniq though could be uuid
 *          string/unicode author_id
 *   string/unicode credit_id
 *
 * EditionList   - list of Edition()
 *   Edition  - data for a 'save' (ie, version, revision, update, release, etc...)
 *      # credit_id can point to a Credit() which can repr multiple authors
 *      string/unicode credit_id
 *      string  date   (iso8601 date format)
 *
 */

struct CreditData {
    std::string author_name;
    std::string author_date;
    std::string author_url;

};

struct Credits : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    void load_author();
    // json_t *author_data;
    json_t* toJson() override;

    void fromJson(json_t *rootJ) override;

    // TODO: whatever the best c++ way to init this is
    Credits() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
        load_author();

    }

    std::string author_name;
    std::string author_date;
    std::string author_url;

    // CreditData credits[1];
    std::vector<CreditData*> vcredits;

    int num_credits = 0;
    // TODO: from/to json
    //       reuse author model and encode
    // credits_path = "credits.json"
    //
    // toJson
    //    serialize list of authors to 'authors' key
    //
    // fromJson
    //    deserialize list of authors
};


json_t* Credits::toJson() {
    debug("to_json");
    json_t *rootJ = json_object();

    json_t *vcreditsJ = json_array();

    for (CreditData *vcredit : vcredits) {
        debug("vcredits vcredit.author_name: %s", vcredit->author_name.c_str());
        json_t *vcredit_dataJ = json_object();

        json_object_set_new(vcredit_dataJ, "name", json_string(vcredit->author_name.c_str()));
        json_object_set_new(vcredit_dataJ, "date", json_string(vcredit->author_date.c_str()));
        json_object_set_new(vcredit_dataJ, "url", json_string(vcredit->author_url.c_str()));

        json_array_append_new(vcreditsJ, vcredit_dataJ);
    }


    // json_t *credit1 = json_string(author_name.c_str());
    // json_array_append_new(creditsJ, credit1);

    // json_object_set_new(rootJ, "credits", creditsJ);
    json_object_set_new(rootJ, "vcredits", vcreditsJ);

    json_object_set_new(rootJ, "namee", json_string(author_name.c_str()));
        return rootJ;
    }


void Credits::fromJson(json_t *rootJ) {
    debug("fromJson");
    json_t *nameJ = json_object_get(rootJ, "namee");
    if (nameJ) {
        author_name = json_string_value(nameJ);
        // setModel(json_string_value(nameJ));
    }

    json_t *vcreditsJ = json_object_get(rootJ, "vcredits");
    if (!json_is_array(vcreditsJ)) {
        warn("JSON parsing error 'vcredits' is not an array");
        //fprintf(stderr, "error: commit %d: message is not a string\n", i + 1);
        json_decref(rootJ);
    }
    size_t index;
    json_t *valueJ;

    json_array_foreach(vcreditsJ, index, valueJ) {
        json_t *creditJ = json_object();
        json_t *cname = json_object_get(valueJ, "name");
        CreditData *credit_data = new CreditData();

        if (json_is_string(cname)) {
            debug("cname: %s", json_string_value(cname));
            credit_data->author_name = json_string_value(cname);
        }

        json_t *cdate = json_object_get(valueJ, "date");
        if (json_is_string(cdate)) {
            debug("cdate: %s", json_string_value(cdate));
            credit_data->author_date = json_string_value(cdate);
        }

        json_t *curl = json_object_get(valueJ, "url");
        if (json_is_string(curl)) {
            debug("curl: %s", json_string_value(curl));
            credit_data->author_url = json_string_value(curl);
        }
        vcredits.push_back(credit_data);
    }

}



void Credits::load_author() {
    json_error_t json_error;
    char *blob;

    json_t *a_data = json_load_file("/Users/adrian/src/Rack-v0.5/blippy.json", 0, &json_error);

    json_t *authors_list = json_object_get(a_data, "authors");

    // FIXME: just grabbing first element
    json_t *authors_data = json_array_get(authors_list, 0);
    json_t *author_name_data = json_object_get(authors_data, "name");
    json_t *author_date_data = json_object_get(authors_data, "date");

    if (!json_is_string(author_name_data))
    {
        warn("JSON parsing error author_name is not a string");
        //fprintf(stderr, "error: commit %d: message is not a string\n", i + 1);
        json_decref(a_data);
    }

    if (!json_is_string(author_date_data))
    {
        warn("JSON parsing error author_date is not a string");
        //fprintf(stderr, "error: commit %d: message is not a string\n", i + 1);
        json_decref(a_data);
    }

    // FIXME: handle error
    author_name = json_string_value(author_name_data);
    author_date = json_string_value(author_date_data);

    info("author_name: %s", author_name.c_str());
    info("author_date: %s", author_date.c_str());
    blob = json_dumps(authors_data, 0);

    CreditData *cd = new CreditData();
    cd->author_name = author_name;
    cd->author_date = author_date;
    cd->author_url = author_name;

    //credits[0] = *cd;
    vcredits.push_back(cd);
    num_credits++;

    info("blob: %s", blob);

    if (!a_data) {
        warn("JSON parsing error loading file=%s line=%d column=%d error=%s", json_error.source, json_error.line, json_error.column, json_error.text);
        // todo: error handling
    }

}


// TODO: custom text/display widgets?

CreditsWidget::CreditsWidget() {
    Credits *module = new Credits();
    setModule(module);
    setPanel(SVG::load(assetPlugin(plugin, "res/Credits.svg")));

    addChild(createScrew<ScrewSilver>(Vec(5, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 20, 365)));

    float x_start = 5.0;
    float x_offset = 0.0;
    float y_start = 10.0;
    float y_offset = 32.0;
    float x_pos = x_start;
    float y_pos = y_start;

    int widget_index = 0;

    for (CreditData *vcredit : module->vcredits) {
        TextField *author_name;
        TextField *author_date;
        TextField *author_url;

        author_name = new TextField();
        //author_name->text = "Default Author Name";
        author_name->text = vcredit->author_name;;
        author_name->box.pos = Vec(x_pos, y_pos);
        author_name->box.size = Vec(200, 28);
        addChild(author_name);

        widget_index++;
        x_pos = x_start + x_offset;
        y_pos = y_start + (widget_index * y_offset);

        author_date = new TextField();
        author_date->text = vcredit->author_date;;
        author_date->box.pos = Vec(x_pos, y_pos);
        author_date->box.size = Vec(200, 28);
        addChild(author_date);

        widget_index++;
        x_pos = x_start + x_offset;
        y_pos = y_start + (widget_index * y_offset);

        author_url = new TextField();
        author_url->text = "http://default.author.example.com";
        author_url->box.pos = Vec(x_pos, y_pos);
        author_url->box.size = Vec(200, 28);
        addChild(author_url);
    }
}
