#include "ToolbarDialog.h"
#include "../toolbarMenubar/model/ToolbarData.h"
#include "../toolbarMenubar/model/ToolbarModel.h"

#include <config.h>
#include <glib/gi18n-lib.h>

enum {
	COLUMN_STRING, COLUMN_BOLD, COLUMN_POINTER, COLUMN_EDITABLE, N_COLUMNS
};

ToolbarDialog::ToolbarDialog(GladeSearchpath * gladeSearchPath, ToolbarModel * model) :
	GladeGui(gladeSearchPath, "toolbar.glade", "DialogEditToolbar") {

	this->tbModel = model;
	this->selected = NULL;

	GtkTreeIter iter;
	this->model = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER, G_TYPE_BOOLEAN);
	gtk_list_store_append(this->model, &iter);
	gtk_list_store_set(this->model, &iter, COLUMN_STRING, _("Predefined"), COLUMN_BOLD, PANGO_WEIGHT_BOLD, COLUMN_POINTER, NULL, COLUMN_EDITABLE, false, -1);

	ListIterator<ToolbarData *> it = model->iterator();

	while (it.hasNext()) {
		ToolbarData * data = it.next();
		if (data->isPredefined()) {
			gtk_list_store_append(this->model, &iter);
			gtk_list_store_set(this->model, &iter, COLUMN_STRING, data->getName().c_str(), COLUMN_BOLD, PANGO_WEIGHT_NORMAL, COLUMN_POINTER, data,
					COLUMN_EDITABLE, false, -1);
		}
	}

	gtk_list_store_append(this->model, &iter);
	gtk_list_store_set(this->model, &iter, COLUMN_STRING, _("Customized"), COLUMN_BOLD, PANGO_WEIGHT_BOLD, COLUMN_POINTER, NULL, COLUMN_EDITABLE, false, -1);

	ListIterator<ToolbarData *> it2 = model->iterator();

	while (it2.hasNext()) {
		ToolbarData * data = it2.next();
		if (!data->isPredefined()) {
			gtk_list_store_append(this->model, &iter);
			gtk_list_store_set(this->model, &iter, COLUMN_STRING, data->getName().c_str(), COLUMN_BOLD, PANGO_WEIGHT_NORMAL, COLUMN_POINTER, data,
					COLUMN_EDITABLE, true, -1);
		}
	}

	GtkWidget * tree = get("toolbarList");
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(this->model));

	GtkCellRenderer * renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn * column = gtk_tree_view_column_new_with_attributes(_("Toolbars"), renderer, "text", COLUMN_STRING, "weight", COLUMN_BOLD, "editable",
			COLUMN_EDITABLE, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	//gtk_tree_view_set_column_drag_function

	GtkTreeSelection * select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK (treeSelectionChangedCallback), this);

	g_signal_connect(renderer, "edited", (GCallback) treeCellEditedCallback, this);

	g_signal_connect(get("btNew"), "clicked", G_CALLBACK(buttonNewCallback), this);
	g_signal_connect(get("btDelete"), "clicked", G_CALLBACK(buttonDeleteCallback), this);
	g_signal_connect(get("btCopy"), "clicked", G_CALLBACK(buttonCopyCallback), this);

	entrySelected(NULL);
}

ToolbarDialog::~ToolbarDialog() {
	g_object_unref(this->model);
	this->tbModel = NULL;
}

void ToolbarDialog::buttonNewCallback(GtkButton * button, ToolbarDialog * dlg) {
	ToolbarData * data = new ToolbarData(false);
	data->setName(_("New"));
	dlg->addToolbarData(data);
}

void ToolbarDialog::buttonDeleteCallback(GtkButton * button, ToolbarDialog * dlg) {
	if (dlg->selected) {
		dlg->tbModel->remove(dlg->selected);
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(dlg->model), &iter)) {
			do {
				ToolbarData * data = NULL;
				gtk_tree_model_get(GTK_TREE_MODEL(dlg->model), &iter, COLUMN_POINTER, &data, -1);

				if (data == dlg->selected) {
					gtk_list_store_remove(dlg->model, &iter);
					break;
				}
			} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(dlg->model), &iter));
		}

		dlg->entrySelected(NULL);
	}
}

void ToolbarDialog::buttonCopyCallback(GtkButton * button, ToolbarDialog * dlg) {
	if (dlg->selected) {
		ToolbarData * data = new ToolbarData(*dlg->selected);
		dlg->addToolbarData(data);
	}
}

void ToolbarDialog::addToolbarData(ToolbarData * data) {
	this->tbModel->add(data);
	GtkTreeIter iter;
	gtk_list_store_append(this->model, &iter);
	gtk_list_store_set(this->model, &iter, COLUMN_STRING, data->getName().c_str(), COLUMN_BOLD, PANGO_WEIGHT_NORMAL, COLUMN_POINTER, data, COLUMN_EDITABLE,
			true, -1);

	GtkWidget * tree = get("toolbarList");

	GtkTreePath * path = gtk_tree_model_get_path(GTK_TREE_MODEL(this->model), &iter);
	GtkTreeViewColumn * column = gtk_tree_view_get_column(GTK_TREE_VIEW(tree), 0);

	gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path, column, true);
}

void ToolbarDialog::treeCellEditedCallback(GtkCellRendererText * renderer, gchar * pathString, gchar * newText, ToolbarDialog * dlg) {
	GtkTreeIter iter;
	ToolbarData * data = NULL;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(dlg->model), &iter, pathString);
	gtk_tree_model_get(GTK_TREE_MODEL(dlg->model), &iter, COLUMN_POINTER, &data, -1);
	if (data) {
		gtk_list_store_set(dlg->model, &iter, COLUMN_STRING, newText, -1);
		data->setName(newText);
	}
}

void ToolbarDialog::entrySelected(ToolbarData * data) {
	GtkWidget * btCopy = get("btCopy");
	GtkWidget * btDelete = get("btDelete");

	if (data == NULL) {
		gtk_widget_set_sensitive(btCopy, false);
		gtk_widget_set_sensitive(btDelete, false);
	} else {
		gtk_widget_set_sensitive(btCopy, true);
		gtk_widget_set_sensitive(btDelete, !data->isPredefined());
	}

	this->selected = data;
}

void ToolbarDialog::treeSelectionChangedCallback(GtkTreeSelection * selection, ToolbarDialog * dlg) {
	GtkTreeIter iter;
	GtkTreeModel * model = NULL;
	ToolbarData * data = NULL;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, COLUMN_POINTER, &data, -1);
		dlg->entrySelected(data);
	}
}

void ToolbarDialog::show() {
	gtk_dialog_run(GTK_DIALOG(this->window));
	gtk_widget_hide(this->window);
}